/*
 * m25p10a flash driver
 *
 * The M25P10-A is a 1 Mbit (128 Kbit x 8) serial Flash memory, with advanced
 * write protection mechanisms, accessed by a high speed SPI-compatible bus.
 *
 * SPI PROGRAMMING GUIDE (From S5PC100 datasheet)
 * Special Function Register should be set as the following sequence. (nCS manual mode)
 * 1. Set Transfer Type (CPOL & CPHA set).
 * 2. Set Clock configuration register.
 * 3. Set SPI MODE configuration register.
 * 4. Set SPI INT_EN register.
 * 5. Set Packet count configuration register if necessary.
 * 6. Set Tx or Rx Channel on.
 * 7. Set nSSout low to start Tx or Rx operation.
 *		a. Set nSSout Bit to low, then start Tx data writing.
 *		b. If audio chip selection bit is set, should not control nSSout.
 */

#include <linux/sched.h>
#include <linux/kernel.h>   /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>       /* everything... */
#include <linux/errno.h>    /* error codes */
#include <linux/types.h>    /* size_t */
#include <linux/mm.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <asm/irq.h>
#include <linux/clk.h>

/* platform and machine related header */
#include <mach/map.h>
#include <mach/gpio.h>
#include <plat/s5p-spi.h>
#include <plat/spi.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-g3.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>

#include "spi_s3c64xx.h"

#define FLASH_PAGESIZE		256

/* Flash opcodes. */
#define	OPCODE_WREN			0x06	/* Write enable */
#define	OPCODE_WRDA			0x04	/* Write disable */
#define	OPCODE_RDSR			0x05	/* Read status register */
#define	OPCODE_WRSR			0x01	/* Write status register 1 byte */
#define	OPCODE_NORM_READ	0x03	/* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define OPCODE_PP			0x02	/* Page Program (up to 256 bytes) */
#define	OPCODE_BE_4K		0x20	/* Erase 4KiB block */
#define	OPCODE_BE_32K		0x52	/* Erase 32KiB block */
#define	OPCODE_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define	OPCODE_SE			0xd8	/* Sector erase (usually 64KiB) */
#define	OPCODE_RDID			0x9f	/* Read JEDEC ID */

/* Status Register bits. */
#define	SR_WIP			1		/* Write in progress */
#define	SR_WEL			2		/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define	SR_BP0			4		/* Block protect 0 */
#define	SR_BP1			8		/* Block protect 1 */
#define	SR_BP2			0x10	/* Block protect 2 */
#define	SR_SRWD			0x80	/* SR write protect */

/* Define max times to check status register before we give up. */
#define	MAX_READY_WAIT_COUNT	100000
#define	CMD_SIZE		4

#define MANUFACTURER_ID	0x20
#define DEVICE_ID	( 0x20 | (0x11 << 8) )

static int flash_major = 0;
static void __iomem  *spi0_reg_base;

struct m25p10a_ops {
	unsigned char *buf;
	loff_t start;	// where to begin read or write
	int len;
};

static int spi_clk_enable( void )
{
	struct clk *spi_clk;
	struct clk *src_clk;

	src_clk = clk_get( NULL, "pclk" );
	if( IS_ERR(src_clk) ) {
		printk( KERN_WARNING "Unable to acquire clock 'pclk'!!!\n" );
		return -1;
	}
	else {
		printk( KERN_INFO "Got the clock 'pclk'\n " );
	}
	
	if( clk_enable(src_clk) ) {
		printk( KERN_WARNING "Couldn't enable clock 'pclk'\n" );
		return -1;
	}

	spi_clk = clk_get( NULL, "spi" );
	if( IS_ERR(spi_clk) ) {
		printk( KERN_WARNING "Unable to acquire clock 'spi'!!!\n" );
		return -1;
	}
	else {
		printk( KERN_INFO "Got the clock 'spi'\n " );
	}

	if( clk_enable(spi_clk) ) {
		printk( KERN_WARNING "Couldn't enable clock 'spi'\n" );
		return -1;
	}

	return 0;
}

static void spi_cfg_gpio( void )
{
	s3c_gpio_cfgpin( S5PC1XX_GPB(0), S5PC1XX_GPB0_SPI_MISO0 );
	s3c_gpio_cfgpin( S5PC1XX_GPB(1), S5PC1XX_GPB1_SPI_CLK0 );
	s3c_gpio_cfgpin( S5PC1XX_GPB(2), S5PC1XX_GPB2_SPI_MOSI0 );
	s3c_gpio_cfgpin( S5PC1XX_GPB(3), S5PC1XX_GPB3_SPI_CS0 );
	s3c_gpio_setpull( S5PC1XX_GPB(0), S3C_GPIO_PULL_UP );
	s3c_gpio_setpull( S5PC1XX_GPB(1), S3C_GPIO_PULL_UP );
	s3c_gpio_setpull( S5PC1XX_GPB(2), S3C_GPIO_PULL_UP );
	s3c_gpio_setpull( S5PC1XX_GPB(3), S3C_GPIO_PULL_UP );
}

static void enable_clk2slave( void )
{
	int val;

	val = ioread32( spi0_reg_base + S3C64XX_SPI_CLK_CFG );
	val &= ~S3C64XX_SPI_PSR_MASK;
	val |= 0x0;
	val |= (1 << 8);
	iowrite32( val, spi0_reg_base + S3C64XX_SPI_CLK_CFG );
	udelay( 100 );
}

static void disable_clk2slave( void )
{
	int val;

	val = ioread32( spi0_reg_base + S3C64XX_SPI_CLK_CFG );
	val &= ~(1 << 8);
	iowrite32( val, spi0_reg_base + S3C64XX_SPI_CLK_CFG );
	udelay( 100 );
}

static void disable_chip( void )
{
	int val;

	val = readl( spi0_reg_base + S3C64XX_SPI_SLAVE_SEL );
	val |= 0x1;	// set nSSOUT to 1 to disable salve chip
	writel( val, spi0_reg_base + S3C64XX_SPI_SLAVE_SEL );
	udelay( 100 );
}

static void enable_chip( void )
{
	int val;

	val = readl( spi0_reg_base + S3C64XX_SPI_SLAVE_SEL );
	val &= ~0x1;	// set nSSOUT to 0 to enable salve chip
	writel( val, spi0_reg_base + S3C64XX_SPI_SLAVE_SEL );
	udelay( 100 );
}

static void basic_spi_cfg( void )
{
	int val;

	/* software reset */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val |= S3C64XX_SPI_CH_SW_RST;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );

	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~S3C64XX_SPI_CH_SW_RST;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );

	/* set transfer type (Polarity and Phase) */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~(S3C64XX_SPI_CH_SLAVE | S3C64XX_SPI_CPOL_L | S3C64XX_SPI_CPHA_B);
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );

	/* both rx and tx channel should be off */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~( S3C64XX_SPI_CH_RXCH_ON | S3C64XX_SPI_CH_TXCH_ON );
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );

	enable_clk2slave();

	/* set spi mode */
	val = readl( spi0_reg_base + S3C64XX_SPI_MODE_CFG );
	val &= ~(S3C64XX_SPI_MODE_BUS_TSZ_MASK | S3C64XX_SPI_MODE_CH_TSZ_MASK);
	val |= S3C64XX_SPI_MODE_BUS_TSZ_BYTE;
	val |= S3C64XX_SPI_MODE_CH_TSZ_BYTE;	// Always 8bits wide
	writel( val, spi0_reg_base + S3C64XX_SPI_MODE_CFG );
}

static void transfer_data( unsigned char *buf, int len )
{
	int i, val;

	/* set tx channel on and rx channel off */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~S3C64XX_SPI_CH_RXCH_ON;
	val |= S3C64XX_SPI_CH_TXCH_ON;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );
	udelay( 1000 );

	for( i = 0; i < len; i++ ) {
		/* select slave flash chip and start tx */
		writeb( buf[i], spi0_reg_base + S3C64XX_SPI_TX_DATA );
		udelay( 1000 );
	}
//	val = readl( spi0_reg_base + S3C64XX_SPI_STATUS );
//	printk( "transfer %d bytes\n", (val >> 6) & 0x7f );
	
	/* disable tx channel */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~S3C64XX_SPI_CH_TXCH_ON;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );
}

static void recv_data( unsigned char *buf, int len )
{
	int i, val;
	
	/* disable tx and start rx */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~S3C64XX_SPI_CH_TXCH_ON;
	val |= S3C64XX_SPI_CH_RXCH_ON;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );
	udelay( 100 );

	/* read and print what we read */
	for( i = 0; i < len; i++ ) {
		buf[ i ] = readb( spi0_reg_base + S3C64XX_SPI_RX_DATA );
		udelay( 100 );
		//printk( KERN_INFO "0x%02x\n", buf[ i ] );
	}

	/* disable rx channel */
	val = readl( spi0_reg_base + S3C64XX_SPI_CH_CFG );
	val &= ~S3C64XX_SPI_CH_RXCH_ON;
	writel( val, spi0_reg_base + S3C64XX_SPI_CH_CFG );

//	val = readl( spi0_reg_base + S3C64XX_SPI_STATUS );
//	printk( "receive %d bytes\n", (val >> 13) & 0x7f );
}

static void check_flash_id( void )
{
	unsigned char buf[ 3 ];
	int m_id, device_id;

	buf[ 0 ] = OPCODE_RDID;
	transfer_data( buf, 1 );
	recv_data( buf, 3 );
	m_id = buf[ 0 ];
	device_id = buf[1] | (buf[2] << 8);

	if( m_id != MANUFACTURER_ID || device_id != DEVICE_ID ) {
		printk( "Flash ID is not right!!!\n" );
	}
	else {
		printk( "Flash ID is right!!!\n" );
	}
}

/*
 * Set write enable latch bit;
 * This bit must be set prior to every Page Programing, 
 * Sector Erase, Bulk Erase and	Write Status Register instruction;
 */
static void write_enable( void )
{
	unsigned char buf[ 1 ];

	enable_chip();
	buf[ 0 ] = OPCODE_WREN;
	transfer_data( buf, 1 );
	disable_chip();
}

/*
 * Set write enable latch bit;
 * This bit must be set prior to every Page Programing, 
 * Sector Erase, Bulk Erase and	Write Status Register instruction;
 */
static void write_disable( void )
{
	unsigned char buf[ 1 ];

	enable_chip();
	buf[ 0 ] = OPCODE_WRDA;
	transfer_data( buf, 1 );
	disable_chip();
}

/*
 * Check Write-In-Progress bit in Read Status Register
 * to see if write finished
 */
static void wait_till_write_finished( void )
{
	unsigned char buf[ 1 ];

	enable_chip();
	buf[ 0 ] = OPCODE_RDSR;
	transfer_data( buf, 1 );

	while( 1 ) {
		recv_data( buf, 1 );
		if( buf[ 0 ] & 0x1 ) {
			//printk( "Write is still in progress\n" );
		}
		else {
			printk( "Write is finished.\n" );
			break;
		}
	}

	disable_chip();
}

/*
 * Read Status Register's Value
 * to see if something happens
 */
static void read_sr( void )
{
	unsigned char buf[ 1 ];

	enable_chip();
	buf[ 0 ] = OPCODE_RDSR;
	transfer_data( buf, 1 );

	recv_data( buf, 1 );
	printk( "Status Register = 0x%x\n", buf[ 0 ] );

	disable_chip();
}

/*
 * Write Status Register's Value
 * to make some change
 */
static void write_sr( unsigned char val )
{
	unsigned char buf[ 2 ];

	enable_chip();
	buf[ 0 ] = OPCODE_WRSR;
	buf[ 1 ] = val;
	transfer_data( buf, 2 );
	udelay( 1000 );

	disable_chip();
}

/*
 * Erase the entire chip
 */
static void erase_chip( void )
{
	unsigned char buf[ 1 ];

	enable_chip();
	buf[ 0 ] = OPCODE_CHIP_ERASE;
	transfer_data( buf, 1 );
	disable_chip();
}

/*
 * Erase the one sector of chip
 */
static void sector_erase( loff_t addr )
{
	unsigned char opcode[ 4 ];

	enable_chip();
	opcode[ 0 ] = OPCODE_SE;
	opcode[ 1 ] = addr >> 16;
	opcode[ 2 ] = addr >> 8;
	opcode[ 3 ] = addr;

	transfer_data( opcode, 4 );
	disable_chip();
}

/*
 * Write data to flash
 */
static void flash_write( loff_t to, size_t len, unsigned char * buf )
{
	unsigned char opcode[ 4 ];

	opcode[ 0 ] = OPCODE_PP;
	opcode[ 1 ] = to >> 16;
	opcode[ 2 ] = to >> 8;
	opcode[ 3 ] = to;

	enable_chip();
	transfer_data( opcode, 4 );	
	transfer_data( buf, len );	
	disable_chip();
}

/*
 * Read data from flash
 */
static void flash_read( loff_t from, size_t len, unsigned char * buf )
{
	int i;
	unsigned char opcode[ 4 ];

	opcode[ 0 ] = OPCODE_NORM_READ;
	opcode[ 1 ] = from >> 16;
	opcode[ 2 ] = from >> 8;
	opcode[ 3 ] = from;

	enable_chip();
	transfer_data( opcode, 4 );	
	recv_data( buf, len );
	disable_chip();

	printk( "Data read from flash is: \n" );
	for( i = 0; i < len; i++ ) {
		printk( "0x%02x\n", buf[ i ] );
	}
}

static int m25p10a_open( struct inode *inode, struct file *filp )
{
	int i, result;

	spi0_reg_base = ioremap( S5PC1XX_PA_SPI0, 0x100 );
	if( NULL == spi0_reg_base ) {
		printk( KERN_ERR "Unable to remap I/O\n" );
		return -ENXIO;
	}

	spi_cfg_gpio();

	result = spi_clk_enable();
	if (result < 0) {
		printk( KERN_ERR "enable spi clock failed.\n" );
		return result;
	}

	for( i = 0; i < 3; i++ ) {
		disable_chip();
		basic_spi_cfg();
		enable_chip();
		check_flash_id();
	}

	return 0;
}

static int m25p10a_close( struct inode *inode, struct file *filp )
{
	/* release I/O mem */
	iounmap( spi0_reg_base );
	return 0;
}

static ssize_t m25p10a_read( struct file *filp, char __user *buffer, 
		size_t count, loff_t *offset )
{
	struct m25p10a_ops *read_info = ( struct m25p10a_ops * )buffer;
	unsigned char *kbuf;

	kbuf = kzalloc( read_info->len, GFP_KERNEL );
	if( !kbuf ) {
		return -ENOMEM;
	}
	//basic_spi_cfg();
	//write_enable();
	//sector_erase( 0 );
	//wait_till_write_finished();

	basic_spi_cfg();
	flash_read( read_info->start, read_info->len, kbuf );

	if( copy_to_user(read_info->buf, kbuf, read_info->len) ) {
		printk( "Error occurs when copy data from user space\n" );
		return -EFAULT;
	}

	return read_info->len;
}

static ssize_t m25p10a_write( struct file *filp, char __user *buffer, 
		size_t count, loff_t *offset )
{
	int i;
	struct m25p10a_ops *write_info = ( struct m25p10a_ops * )buffer;
	unsigned char *kbuf;
	unsigned char rbuf[ 10 ];

	kbuf = kzalloc( write_info->len, GFP_KERNEL );
	if( !kbuf ) {
		return -ENOMEM;
	}

	if( copy_from_user(kbuf, write_info->buf, write_info->len) ) {
		printk( "Error occurs when copy data from user space\n" );
		return -EFAULT;
	}
	for( i = 0; i < write_info->len; i++ ) {
		printk( "Kbuf: 0x%02x\n", kbuf[i] );
	}

	read_sr();
	basic_spi_cfg();
	write_enable();
	erase_chip();
	//sector_erase( 0 );
	wait_till_write_finished();

	basic_spi_cfg();
	flash_read( write_info->start, write_info->len, rbuf );

	basic_spi_cfg();
	write_enable();
	flash_write( write_info->start, write_info->len, kbuf );
	wait_till_write_finished();

	return write_info->len;
}

static int m25p10a_ioctl( struct inode *inode, struct file *filp, 
		unsigned int cmd, unsigned long arg )
{
	return 0;
}

static struct file_operations flash_ops = {
	.owner	 = THIS_MODULE,
	.open    = m25p10a_open,
	.release = m25p10a_close,
	.read    = m25p10a_read,
	.write   = m25p10a_write,
	.ioctl   = m25p10a_ioctl,
};

static int flash_init(void)
{
	int i, result;
	unsigned char buf[ 5 ] = { 0 };
	result = register_chrdev( flash_major, "spi-flash-m25p10a", &flash_ops );
	if (result < 0) {
		printk( KERN_ERR "Register char device failed.\n" );
		return result;
	}
	else {
		flash_major = result;
		printk( "Major number of m25p10a is %d\n", flash_major );
	}

#if 0
	/* test function of read and write to flash */
	basic_spi_cfg();
	write_enable();
	//erase_chip();
	sector_erase( 0 );
	wait_till_write_finished();
	write_disable();
	read_sr();

	//basic_spi_cfg();
	//read_sr();
	basic_spi_cfg();
	write_enable();
	write_sr( 0x0 );	// clear Block Proteced bits because 
						// after Bulk Erase they will be set to '1'
						// which stop us from writting data to the flash
	wait_till_write_finished();
	write_disable();
	read_sr();

	basic_spi_cfg();
	flash_read( 0, 5, buf );

	for( i = 0; i < 5; i++ ) {
		buf[ i ] = 0x60 + i;
	}
	basic_spi_cfg();
	write_enable();
	flash_write( 0, 5, buf );
	wait_till_write_finished();

	for( i = 0; i < 5; i++ ) {
		buf[ i ] = 0;
	}
	basic_spi_cfg();
	flash_read( 0, 5, buf );
#endif

	printk(KERN_INFO "SPI Flash M25P10A installed \n");
	return 0;
}

static void flash_cleanup(void)
{
	unregister_chrdev( flash_major, "spi-flash-m25p10a" );
	printk(KERN_INFO "M25P10A flash driver uninstalled...\n");
}

module_init( flash_init );
module_exit( flash_cleanup );
MODULE_LICENSE( "GPL" );
