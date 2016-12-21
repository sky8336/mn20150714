#include <stdio.h>
#include <fcntl.h>

struct m25p10a_ops {
	unsigned char *buf;
	loff_t start;	// where to begin read or write
	int len;
};

int
main( int argc, char *argv[] )
{
	int i, nread, nwrite;
	struct m25p10a_ops ops;
	unsigned char buf[ 10 ];
	int fd = open( "/dev/m25p10a", O_RDWR );
	if( fd < 0 ) {
		perror( "open /dev/m25p10a error" );
		return -1;
	}
	
	for( i = 0; i < 10; i++ ) {
		buf[ i ] = 0x39;
	}

	nwrite = 0;
	ops.buf = buf;
	ops.len = 10;
	ops.start = 0;
	nwrite = write( fd, (void *)&ops, sizeof( ops ) );
	if( nwrite < 0 ) {
		perror( "write error" );
		return -1;
	}
	printf( "nwrite = %d\n", nwrite );

	for( i = 0; i < 10; i++ ) {
		ops.buf[ i ] = 0;
	}

	nread = read( fd, (void *)&ops, sizeof( ops ) );
	if( nread < 0 ) {
		perror( "read error" );
		return -1;
	}
	printf( "nread = %d\n", nread );
	for( i = 0; i < 10; i++ ) {
		printf( "0x%02x\n", ops.buf[i] );
	}

	close( fd );
	return 0;
}
