#!/bin/bash

FORCE=n
QUIET=n
BINARY=n
BRANCH=
TARGET=".diffdir"
CURRENT_BRANCH=

# Tell the usage of this script.
# no parameters.
usage()
{
	echo "Usage: `basename $0` [-nqf] [-t TARGET] -b BRANCH"
	echo "  -n  set to output binary diff."
	echo "  -q  quiet mode, minimal messges."
	echo "  -f  force mode, none confirmation."
	echo "  -t  output target directory."
	echo "  -b  the base branch to diff from."
	exit 1
}

# Check whether the current directory is a git repository.
# no parameters.
isrepo()
{
	status=`git status -s`;
	if [ ! -z "$status" ]; then
		echo "Not a repository or unstaged.";
		exit 1;
	fi
}

# Get current branch/tag/commit name.
# no parameters.
getcurrBranch()
{
	CURRENT_BRANCH=`git branch | \
		awk -F " " '$1=="*" && substr($2,0,1)!="(" {print $2} $1=="*" \
		&& substr($2,0,1)=="(" {print substr($4,0,length($4)-1)}'`;
	if [ -z "$CURRENT_BRANCH" ]; then
		echo "Get current branch failed.";
		exit 1;
	fi
}
# Ask user to confirm.
# no parameters.
confirm()
{
	if [ "$FORCE" != "y" ]; then
		read -p "Confirm $1? y/n default(y) : ";
		ch="$REPLY"
		if [ "$ch" == "y" -o "$ch" == "" ]; then
			return 0;
		fi
	else
		return 0;
	fi
	return 1;
}

# Adaptable echo
# $1 : message.
nqecho()
{
	if [ "$QUIET" != "y" ]; then
		echo "$1";
	fi
	return 0;
}


isrepo; # Current workdir is a repo.
getcurrBranch; # ##
[ $# -eq 0 ] && usage # At least one patameter required.

# Resolve parameters.
while getopts :fnqt:b: OPTION
do
	case $OPTION in
		f)
			FORCE=y
			;;
		q)
			QUIET=y
			;;
		n)
			BINARY=y
			;;
		b)
			BRANCH=$OPTARG
			;;
		t)
			TARGET=$OPTARG
			;;
		\?)
			usage
			;;
	esac
done

shift $(($OPTIND - 1)) # abandon parameters processed.

if [ -z "$BRANCH" ]; then # parameter -b required.
	echo "You must specify base BRANCH with -b option";
	usage;
fi

# Target dir generating.
mkdir -p "$TARGET";
if [ ! -d "$TARGET" ]; then
	echo "$TARGET is not a diretory";
	exit 1;
fi
if [ -d "$TARGET/__OUT" ]; then
	rm -fr "$TARGET/__OUT";
fi
if mkdir -p "$TARGET/__OUT"; then
	TARGET="$TARGET/__OUT";
else
	echo "Create directory [$TARGET/__OUT] failed."
	exit 1;
fi
odir=`pwd`;
cd "$TARGET";
TARGET=`pwd`;
cd "$odir";

# Generating change list.
FILE_DIFF=`git diff $BRANCH --name-status --no-renames`;
ADD_LIST=`echo "$FILE_DIFF" | awk -F "\t" '$1=="A" {print $2}'`;
MOD_LIST=`echo "$FILE_DIFF" | awk -F "\t" '$1=="M" {print $2}'`;
DEL_LIST=`echo "$FILE_DIFF" | awk -F "\t" '$1=="D" {print $2}'`;

# Ask user confirm all the changes.
index=0;
for line in `echo "$ADD_LIST"`; do
	if confirm "addition of [$line]"; then
		ADD[index]="$line";
		index=$(($index+1));
	fi
done
index=0;
for line in `echo "$MOD_LIST"`; do
	if confirm "modification of [$line]"; then
		MOD[index]="$line";
		index=$(($index+1));
	fi
done
index=0;
for line in `echo "$DEL_LIST"`; do
	if confirm "deletion of [$line]"; then
		DEL[index]="$line";
		index=$(($index+1));
	fi
done

# Print the confirmed changes.
echo "All changes lists below: "
echo "Additions:"
for file in ${ADD[@]}; do
	echo $file;
done
echo "Modifications:"
for file in ${MOD[@]}; do
	echo $file
done
echo "Deletions:"
for file in ${DEL[@]}; do
	echo $file
done

# Generating diff package.
if confirm "changes upon"; then
	echo "Copying files..."
	# before
	if [ ${#MOD[@]} -gt 0 -o ${#DEL[@]} -gt 0 ]; then
		mkdir -p "$TARGET/before/";
		git archive "$BRANCH" --format=tar ${MOD[@]} ${DEL[@]} | tar -x -C "$TARGET/before/";
	fi
	# after
	if [ ${#ADD[@]} -gt 0 -o ${#MOD[@]} -gt 0 ]; then
		mkdir -p "$TARGET/after/";
		git archive "$CURRENT_BRANCH" --format=tar ${ADD[@]} ${MOD[@]} | tar -x -C "$TARGET/after/";
	fi
	# Generating before script, which to copy before files from SCM checkout directory.
	if [ ${#MOD[@]} -gt 0 -o ${#DEL[@]} -gt 0 ]; then
		echo "Generating before copying script...";
		echo '#! /bin/bash' > "$TARGET/bef.sh";
		echo 'odir=`pwd`' >> "$TARGET/bef.sh";
		echo 'cd "$1"' >> "$TARGET/bef.sh";
		echo 'SOURCE=`pwd`' >> "$TARGET/bef.sh";
		echo 'cd "$odir"' >> "$TARGET/bef.sh";
		echo 'odir=`pwd`' >> "$TARGET/bef.sh";
		echo 'cd "$2"' >> "$TARGET/bef.sh";
		echo 'TARGET=`pwd`' >> "$TARGET/bef.sh";
		echo 'cd "$odir"' >> "$TARGET/bef.sh";
		for file in ${MOD[@]}; do
			echo 'mkdir -p $(dirname "$TARGET/'"$file\")" >> "$TARGET/bef.sh";
			echo 'cp -f "$SOURCE/'"$file\""' "$TARGET/'"$file\"" >> "$TARGET/bef.sh";
		done
		for file in ${DEL[@]}; do
			echo 'mkdir -p $(dirname "$TARGET/'"$file\")" >> "$TARGET/bef.sh";
			echo 'cp -f "$SOURCE/'"$file\""' "$TARGET/'"$file\"" >> "$TARGET/bef.sh";
		done
		echo "$TARGET/bef.sh"
	fi
	# Generating delete script, which to delete the corresponding files from SCM checkout directory.
	if [ ${#DEL[@]} -gt 0 ]; then
		echo "Generating deleting script...";
		echo '#! /bin/bash' > "$TARGET/del.sh";
		echo 'odir=`pwd`' >> "$TARGET/del.sh";
		echo 'cd "$1"' >> "$TARGET/del.sh";
		echo 'TARGET=`pwd`' >> "$TARGET/del.sh";
		echo 'cd "$odir"' >> "$TARGET/del.sh";
		for file in ${DEL[@]}; do
			echo 'rm -f "$TARGET/'"$file\"" >> "$TARGET/del.sh";
		done
		echo "$TARGET/del.sh";
	fi
	git diff "$BRANCH" > "$TARGET/diff.patch"
	if [ "$BINARY" == "y" ]; then
		git diff --binary "$BRANCH" > "$TARGET/diff_b.patch"
	fi
fi
echo "";
echo "Done.";
