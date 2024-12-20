#ifndef _ALTERA_DMA_H
#define _ALTERA_DMA_H

#define ALTERA_DMA_DRIVER_NAME    "Altera DMA"
#define ALTERA_DMA_DEVFILE        "altera_dma"

#define ALTERA_DMA_BAR_NUM (6)
#define ALTERA_DMA_DESCRIPTOR_NUM 128

#define ALTERA_DMA_WR_DMA_CTRL          0x0000
#define ALTERA_DMA_WR_DESC_STATUS       0x0004
#define ALTERA_DMA_WR_RC_DESC_BASE_LOW  0x0008
#define ALTERA_DMA_WR_RC_DESC_BASE_HIGH 0x000C
#define ALTERA_DMA_WR_LAST_DESC_IDX     0x0010
#define ALTERA_DMA_WR_EP_DESC_BASE_LOW  0x0014
#define ALTERA_DMA_WR_EP_DESC_BASE_HIGH 0x0018
#define ALTERA_DMA_WR_DMA_PERF          0x001C

#define ALTERA_DMA_RD_DMA_CTRL          0x0100
#define ALTERA_DMA_RD_DESC_STATUS       0x0104
#define ALTERA_DMA_RD_RC_DESC_BASE_LOW  0x0108
#define ALTERA_DMA_RD_RC_DESC_BASE_HIGH 0x010C
#define ALTERA_DMA_RD_LAST_DESC_IDX     0x0110
#define ALTERA_DMA_RD_EP_DESC_BASE_LOW  0x0114
#define ALTERA_DMA_RD_EP_DESC_BASE_HIGH 0x0118
#define ALTERA_DMA_RD_DMA_PERF          0x011C

#define ALTERA_LITE_DMA_RD_RC_LOW_SRC_ADDR      0x0000
#define ALTERA_LITE_DMA_RD_RC_HIGH_SRC_ADDR     0x0004
#define ALTERA_LITE_DMA_RD_CTLR_LOW_DEST_ADDR   0x0008
#define ALTERA_LITE_DMA_RD_CTRL_HIGH_DEST_ADDR  0x000C
#define ALTERA_LITE_DMA_RD_LAST_PTR             0x0010

#define ALTERA_LITE_DMA_WR_RC_LOW_SRC_ADDR      0x0100
#define ALTERA_LITE_DMA_WR_RC_HIGH_SRC_ADDR     0x0104
#define ALTERA_LITE_DMA_WR_CTLR_LOW_DEST_ADDR   0x0108
#define ALTERA_LITE_DMA_WR_CTRL_HIGH_DEST_ADDR  0x010C
#define ALTERA_LITE_DMA_WR_LAST_PTR             0x0110

#define ALTERA_EPLAST_DIFF              1
#define ALTERA_DMA_NUM_DWORDS           512
#define DESC_CTRLLER_BASE               0x0000
#define CTL_STS_BITS                    0x0100
#define TIMEOUT_THRESH                  0xFFFF
#define MAX_NUM_DWORDS                  0x7FFFF//4096

//Arria 10
#ifdef ARRIA10
#warning "Defined ARRIA10 Sucessful"
#define ONCHIP_MEM_BASE                 	0x10000000
#define ONCHIP_MEM_DESC_MEM_BASE        	0x10000000
#define OFFCHIP_MEM_BASE			0x00000000
#define RD_CTRL_BUF_BASE_LOW			0x80000000
#define RD_CTRL_BUF_BASE_HI			0x80001FFF
#define WR_CTRL_BUF_BASE_LOW			0x80002000
#define WR_CTRL_BUF_BASE_HI			0x80003FFF
#define BAR 2
#define DMAMASK 64
#endif

//Cyclone 10
#ifdef CYCLONE10
#warning "Defined CYCLONE10 Sucessful"
#define ONCHIP_MEM_BASE                 	0x40000000
#define ONCHIP_MEM_DESC_MEM_BASE        	0x40000000
#define OFFCHIP_MEM_BASE			0x00000000
#define RD_CTRL_BUF_BASE_LOW			0x80000000
#define RD_CTRL_BUF_BASE_HI			0x80001FFF
#define WR_CTRL_BUF_BASE_LOW			0x80002000
#define WR_CTRL_BUF_BASE_HI			0x80003FFF
#define BAR 2
#define DMAMASK 64
#endif

//Stratix 10
#ifdef STRATIX10
#warning "Defined STRATIX10 Sucessful"
#define ONCHIP_MEM_BASE                		0x00000000
#define ONCHIP_MEM_DESC_MEM_BASE        	0x00000000
#define OFFCHIP_MEM_BASE			0x80000000
#define RD_CTRL_BUF_BASE_LOW			0x00010000
#define RD_CTRL_BUF_BASE_HI			0x0001FFFF
#define WR_CTRL_BUF_BASE_LOW			0x00020000
#define WR_CTRL_BUF_BASE_HI			0x0002FFFF
#define BAR 4
#define DMAMASK 32
#endif

//Stratix V
#ifdef STRATIXV
#warning "Defined STRATIXV Sucessful"
#define ONCHIP_MEM_BASE                 	0x08000000
#define ONCHIP_MEM_DESC_MEM_BASE        	0x08000000
#define OFFCHIP_MEM_BASE			0x00000000
#define RD_CTRL_BUF_BASE_LOW			0x08010000
#define RD_CTRL_BUF_BASE_HI			0x08011FFF
#define WR_CTRL_BUF_BASE_LOW			0x08012000
#define WR_CTRL_BUF_BASE_HI			0x08013FFF
#define BAR 4
#define DMAMASK 64
#endif


struct dma_descriptor {
    u32 src_addr_ldw;
    u32 src_addr_udw;
    u32 dest_addr_ldw;
    u32 dest_addr_udw;
    u32 ctl_dma_len;
    u32 reserved[3];
} __attribute__ ((packed));

struct dma_header {
    u32 eplast;
    u32 reserved[7];    
} __attribute__ ((packed));

struct dma_desc_table {
    struct dma_header header;
    struct dma_descriptor descriptors[ALTERA_DMA_DESCRIPTOR_NUM];
} __attribute__ ((packed));

struct lite_dma_header {
    volatile u32 flags[128];
} __attribute__ ((packed));


struct lite_dma_desc_table {
    struct lite_dma_header header;
    struct dma_descriptor descriptors[ALTERA_DMA_DESCRIPTOR_NUM];
} __attribute__ ((packed));

struct altera_pcie_dma_bookkeep {
    struct pci_dev *pci_dev;

    u8 revision;
    u8 irq_pin;
    char msi_enabled;
    u8 irq_line;
    char dma_capable;

    void * __iomem bar[ALTERA_DMA_BAR_NUM];
    size_t bar_length[ALTERA_DMA_BAR_NUM];

    struct dma_desc_table *table_rd_cpu_virt_addr;
    struct dma_desc_table *table_wr_cpu_virt_addr;
    struct lite_dma_desc_table *lite_table_rd_cpu_virt_addr;
    struct lite_dma_desc_table *lite_table_wr_cpu_virt_addr;
 
    dma_addr_t lite_table_rd_bus_addr; 
    dma_addr_t table_rd_bus_addr; 
    dma_addr_t lite_table_wr_bus_addr;
    dma_addr_t table_wr_bus_addr;

    int numpages;
    u8 *rp_rd_buffer_virt_addr;
    dma_addr_t rp_rd_buffer_bus_addr;
    u8 *rp_wr_buffer_virt_addr;
    dma_addr_t rp_wr_buffer_bus_addr;

    dev_t cdevno;
    struct cdev cdev;

    int user_pid;
    struct task_struct *user_task;
    wait_queue_head_t wait_q;
    atomic_t status;

    struct dma_status dma_status;
};

static int scan_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev) __init;
static int map_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev) __init;
static int dma_test(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev);
//static irqreturn_t dma_isr(int irq, void *dev_id);

static int altera_pci_probe(struct pci_dev *dev, const struct pci_device_id *id) __init;
//static int set_table_header(struct dma_header *header, u32 eplast);
//static int print_table_header(struct dma_header *header);
static int set_read_desc(struct dma_descriptor *rd_desc, dma_addr_t source, u64 dest, u32 ctl_dma_len, u32 id);
//static int print_desc(struct dma_descriptor *desc);
static int set_write_desc(struct dma_descriptor *wr_desc, u64 source, dma_addr_t dest, u32 ctl_dma_len, u32 id);
static int scan_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev);
static int init_rp_mem(u8 *rp_buffer_virt_addr, u32 num_dwords, u32 init_value, u8 increment);
static int rp_compare(u8 *virt_addr1, u8 *virt_addr2, u32 num_dwords);
static int rp_ep_compare(u8 *virt_addr, struct altera_pcie_dma_bookkeep *bk_ptr, u32 mem_byte_offset, u32 num_dwords);
//static int print_ep_data(struct altera_pcie_dma_bookkeep *bk_ptr, u32 mem_byte_offset, u32 num_dwords);
//static int print_rp_data(u8 *virt_addr, u32 num_dwords);
static int init_ep_mem(struct altera_pcie_dma_bookkeep *bk_ptr, u32 mem_byte_offset, u32 num_dwords, u32 init_value, u8 increment);
static void altera_pci_remove(struct pci_dev *dev) __exit;
//static int eplast_busy_wait(struct altera_pcie_dma_bookkeep *bk_ptr, u32 expected_eplast, u8 rw);
static int diff_timeval(struct timespec64 *result, struct timespec64 *t2, struct timespec64 *t1);
static int init_chrdev (struct altera_pcie_dma_bookkeep *bk_ptr) __init;

ssize_t altera_dma_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t altera_dma_write(struct file *file, char __user *buf, size_t count, loff_t *pos);
ssize_t altera_dma_rw(struct file *file, char __user *buf, size_t count, loff_t *pos, int read);
int altera_dma_open(struct inode *inode, struct file *file);
int altera_dma_release(struct inode *inode, struct file *file);
ssize_t altera_dma_exec_cmd(struct dma_cmd *ucmd, struct altera_pcie_dma_bookkeep *bk_ptr);
static long altera_dma_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

// lite
static int set_lite_table_header(struct lite_dma_header *header);


#endif /* _ALTERA_DMA_H */
