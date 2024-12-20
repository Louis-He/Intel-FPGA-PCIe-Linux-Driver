#include <linux/time.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/random.h>
#include "altera_dma_cmd.h"
#include "altera_dma.h"
// #include <linux/unistd.h>

#define TIMEOUT 0x2000000


static int diff_timeval(struct timespec64 *result, struct timespec64 *t2, struct timespec64 *t1)
{
    long long int diff = (t2->tv_nsec + 1000000000 * t2->tv_sec) - (t1->tv_nsec + 1000000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000000;
    result->tv_nsec = diff % 1000000000;
    return ( diff < 0 );
}

static long altera_dma_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct altera_pcie_dma_bookkeep *bk_ptr = filp->private_data;
    switch (cmd) {
        case ALTERA_IOCX_START:
            dma_test(bk_ptr, bk_ptr->pci_dev);
        case ALTERA_CMD_WAIT_DMA: 
            wait_event_interruptible(bk_ptr->wait_q, !atomic_read(&bk_ptr->status));
    }
    return -EINVAL;
}

ssize_t altera_dma_read(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    if (altera_dma_rw(file, buf, count, pos, 1) < 0)
        return -1;
    return count;
}

ssize_t altera_dma_write(struct file *file, char __user *buf, size_t count, loff_t *pos) {
    if (altera_dma_rw(file, buf, count, pos, 0) < 0)
        return -1;
    return count;
}


ssize_t altera_dma_rw(struct file *file, char __user *buf, size_t count, loff_t *pos, int read) {

    struct dma_cmd __user *ucmd_p = (struct dma_cmd *)buf;
    struct altera_pcie_dma_bookkeep *bk_ptr = file->private_data;
    return altera_dma_exec_cmd(ucmd_p, bk_ptr);
}

ssize_t altera_dma_exec_cmd(struct dma_cmd *ucmd, struct altera_pcie_dma_bookkeep * bk_ptr) {
    int rc, num_input;
    struct dma_cmd kcmd;
    struct dma_status curr;
    if (copy_from_user (&kcmd, ucmd, sizeof(struct dma_cmd))) {
        rc = -EFAULT;
        return rc;
    }

    switch (kcmd.cmd) {
        case ALTERA_CMD_ENA_DIS_READ: {
                                          bk_ptr->dma_status.run_read = !bk_ptr->dma_status.run_read;
                                          break;
                                      }
        case ALTERA_CMD_ENA_DIS_WRITE: {
                                           bk_ptr->dma_status.run_write = !bk_ptr->dma_status.run_write;
                                           break;
                                       }
        case ALTERA_CMD_ENA_DIS_SIMUL: {
                                           bk_ptr->dma_status.run_simul = !bk_ptr->dma_status.run_simul;
                                           break;
                                       }
        case ALTERA_CMD_MODIFY_NUM_DWORDS: {
                                               if (copy_from_user (&num_input, kcmd.buf, sizeof(int))) {
                                                   rc = -EFAULT;
                                                   return rc;
                                               }
                                               bk_ptr->dma_status.altera_dma_num_dwords = num_input;
                                               bk_ptr->dma_status.write_time.tv_sec = 0;
                                               bk_ptr->dma_status.read_time.tv_sec = 0;
                                               bk_ptr->dma_status.simul_time.tv_sec = 0;
                                               bk_ptr->dma_status.write_time.tv_nsec = 0;
                                               bk_ptr->dma_status.read_time.tv_nsec = 0;
                                               bk_ptr->dma_status.simul_time.tv_nsec = 0;
                                               break;
                                           }
        case ALTERA_CMD_MODIFY_NUM_DESC: {
                                             if (copy_from_user (&num_input, kcmd.buf, sizeof(int))) {
                                                 rc = -EFAULT;
                                                 return rc;
                                             }
                                             bk_ptr->dma_status.altera_dma_descriptor_num = num_input;
                                             bk_ptr->dma_status.write_time.tv_sec = 0;
                                             bk_ptr->dma_status.read_time.tv_sec = 0;
                                             bk_ptr->dma_status.simul_time.tv_sec = 0;
                                             bk_ptr->dma_status.write_time.tv_nsec = 0;
                                             bk_ptr->dma_status.read_time.tv_nsec = 0;
                                             bk_ptr->dma_status.simul_time.tv_nsec = 0;
                                             break;
                                         }
	case ALTERA_CMD_ONCHIP_OFFCHIP: {
					   bk_ptr->dma_status.onchip = !bk_ptr->dma_status.onchip;
					   break;
					}
	case ALTERA_CMD_RAND: {
					   bk_ptr->dma_status.rand = !bk_ptr->dma_status.rand;
					   break;
					}
        case ALTERA_CMD_READ_STATUS: {
                                         bk_ptr->dma_status.length_transfer = (bk_ptr->dma_status.altera_dma_num_dwords*4*bk_ptr->dma_status.altera_dma_descriptor_num)/1024;
					 curr.onchip = bk_ptr->dma_status.onchip;
					 curr.rand = bk_ptr->dma_status.rand;
                                         curr.run_write = bk_ptr->dma_status.run_write;
                                         curr.run_read = bk_ptr->dma_status.run_read;
                                         curr.run_simul = bk_ptr->dma_status.run_simul;
                                         curr.length_transfer = bk_ptr->dma_status.length_transfer;
                                         curr.write_time = bk_ptr->dma_status.write_time;
                                         curr.read_time = bk_ptr->dma_status.read_time;
                                         curr.simul_time = bk_ptr->dma_status.simul_time;
                                         curr.pass_read = bk_ptr->dma_status.pass_read;
                                         curr.pass_write = bk_ptr->dma_status.pass_write;
                                         curr.pass_simul = bk_ptr->dma_status.pass_simul;
                                         curr.altera_dma_num_dwords = bk_ptr->dma_status.altera_dma_num_dwords;
                                         curr.altera_dma_descriptor_num = bk_ptr->dma_status.altera_dma_descriptor_num;
                                         curr.offset = bk_ptr->dma_status.offset;
                                         curr.read_eplast_timeout = bk_ptr->dma_status.read_eplast_timeout;
                                         curr.write_eplast_timeout = bk_ptr->dma_status.write_eplast_timeout;
                                         if (copy_to_user (kcmd.buf, &curr, sizeof(struct dma_status))) {
                                             rc = -EFAULT;
                                             return rc;
                                         }
                                         break;            
                                     }
        default:
                                     printk(KERN_DEBUG "command issued from user space doesn't exist %d", kcmd.cmd);
    }
    return 0;
}

int altera_dma_open(struct inode *inode, struct file *file) {
    struct altera_pcie_dma_bookkeep *bk_ptr = 0;

    bk_ptr = container_of(inode->i_cdev, struct altera_pcie_dma_bookkeep, cdev);
    file->private_data = bk_ptr;
    bk_ptr->user_pid = current->pid;

    return 0;
}

int altera_dma_release(struct inode *inode, struct file *file) {
    return 0;
}
/*
static irqreturn_t dma_isr(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}
*/
struct file_operations altera_dma_fops = {
    .owner          = THIS_MODULE,
    .read           = altera_dma_read,
    .write          = (void *)altera_dma_write,
    .open           = altera_dma_open,
    .release        = altera_dma_release,
    .unlocked_ioctl = altera_dma_ioctl,
};

static int __init init_chrdev (struct altera_pcie_dma_bookkeep *bk_ptr) {
    int dev_minor = 0;
    int dev_major = 0;
    int devno = -1;

    int result = alloc_chrdev_region(&bk_ptr->cdevno, dev_minor, 1, ALTERA_DMA_DEVFILE);

    dev_major = MAJOR(bk_ptr->cdevno);
    if (result < 0) {
        printk(KERN_DEBUG "cannot get major ID %d", dev_major);
    }

    devno = MKDEV(dev_major, dev_minor);

    cdev_init(&bk_ptr->cdev, &altera_dma_fops);
    bk_ptr->cdev.owner = THIS_MODULE;
    bk_ptr->cdev.ops = &altera_dma_fops;
    result = cdev_add(&bk_ptr->cdev, devno, 1);

    if (result)
        return -1; 
    return 0;
}
/*
static int set_table_header(struct dma_header *header, u32 eplast)
{
    header->eplast = cpu_to_le32(eplast);
    header->reserved[0] = cpu_to_le32(0x0);    
    header->reserved[1] = cpu_to_le32(0x0);    
    header->reserved[2] = cpu_to_le32(0x0);    
    header->reserved[3] = cpu_to_le32(0x0);    
    return 0;
}

static int print_table_header(struct dma_header *header)
{
    printk(KERN_DEBUG "Print Header:"                  );  
    printk(KERN_DEBUG "0x%x\n",    *(u32*)header       );  
    printk(KERN_DEBUG "0x%x\n",    *((u32*)header+0x1) ); 
    printk(KERN_DEBUG "0x%x\n",    *((u32*)header+0x2) );
    printk(KERN_DEBUG "0x%x\n",    *((u32*)header+0x3) );
    printk(KERN_DEBUG "0x%x\n",    *((u32*)header+0x4) );
    return 0;
}
*/
static int set_read_desc(struct dma_descriptor *rd_desc, dma_addr_t source, u64 dest, u32 ctl_dma_len, u32 id)
{
    rd_desc->src_addr_ldw = cpu_to_le32(source & 0xffffffffUL);
    rd_desc->src_addr_udw = cpu_to_le32((source >> 32));
    rd_desc->dest_addr_ldw = cpu_to_le32(dest & 0xffffffffUL);
    rd_desc->dest_addr_udw = cpu_to_le32((dest >> 32));
    rd_desc->ctl_dma_len = cpu_to_le32(ctl_dma_len | (id << 18));
    rd_desc->reserved[0] = cpu_to_le32(0x0);
    rd_desc->reserved[1] = cpu_to_le32(0x0);
    rd_desc->reserved[2] = cpu_to_le32(0x0);
    return 0;
}

/*
   static int print_desc(struct dma_descriptor *desc)
   {

   printk(KERN_DEBUG "Print Desc"                   );  
   printk(KERN_DEBUG "0x%x\n",    *(u32*)desc       );  
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x1) ); 
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x2) );
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x3) );
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x4) );
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x5) );
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x6) );
   printk(KERN_DEBUG "0x%x\n",    *((u32*)desc+0x7) );
   return 0;
   }*/

static int set_write_desc(struct dma_descriptor *wr_desc, u64 source, dma_addr_t dest, u32 ctl_dma_len, u32 id)
{
    wr_desc->src_addr_ldw = cpu_to_le32(source & 0xffffffffUL);
    wr_desc->src_addr_udw = cpu_to_le32((source >> 32));
    wr_desc->dest_addr_ldw = cpu_to_le32(dest & 0xffffffffUL);
    wr_desc->dest_addr_udw = cpu_to_le32((dest >> 32));
    wr_desc->ctl_dma_len = cpu_to_le32(ctl_dma_len | (id << 18));
    wr_desc->reserved[0] = cpu_to_le32(0x0);
    wr_desc->reserved[1] = cpu_to_le32(0x0);
    wr_desc->reserved[2] = cpu_to_le32(0x0);
    return 0;
}

static int scan_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev)
{
    int i;
    for (i = 0; i < ALTERA_DMA_BAR_NUM; i++) {
        unsigned long bar_start = pci_resource_start(dev, i);
        unsigned long bar_end = pci_resource_end(dev, i);
        unsigned long bar_flags = pci_resource_flags(dev, i);
        bk_ptr->bar_length[i] = pci_resource_len(dev, i);
        dev_info(&dev->dev, "BAR[%d] 0x%08lx-0x%08lx flags 0x%08lx, length %d", i, bar_start, bar_end, bar_flags, (int)bk_ptr->bar_length[i]);
    }
    return 0; 
}

static int init_rp_mem(u8 *rp_buffer_virt_addr, u32 num_dwords, u32 init_value, u8 increment)
{
    u32 i = 0;
    u32 increment_value = 0;
    u32 tmp_rand;
    for (i = 0; i < num_dwords; i++) {    
	get_random_bytes(&tmp_rand, sizeof(tmp_rand));
       *((u32*)rp_buffer_virt_addr+i) = cpu_to_le32(tmp_rand);
    }
    return 0;
}

static int rp_ep_compare(u8 *virt_addr, struct altera_pcie_dma_bookkeep *bk_ptr, u32 mem_byte_offset, u32 num_dwords)
{
    u32 i = 0;
    u32 rp_data = 0;
    u32 ep_data = 0;
    u32 j, rp_tmp, ep_tmp;
    u32 count = 1;
    //printk(KERN_DEBUG "RP                      EP");
    for (i = 0; i < num_dwords; i++) {
	if (bk_ptr->dma_status.onchip)
	        ep_data = ioread32((u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+ONCHIP_MEM_BASE)+i);
	else
	 	ep_data = ioread32((u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+OFFCHIP_MEM_BASE)+i);

        rmb();
        rp_data = *((u32*)virt_addr+i); 
	
	
	
	if(ep_data != rp_data){
		udelay(1000);
		if (bk_ptr->dma_status.onchip)
	        	ep_data = ioread32((u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+ONCHIP_MEM_BASE)+i);
		else
	 		ep_data = ioread32((u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+OFFCHIP_MEM_BASE)+i);
	}

        if ( ep_data != rp_data ) {
		if(bk_ptr->dma_status.onchip)
		            printk(KERN_DEBUG "%p: 0x%08x != %p: 0x%08x => Data mismatch", (u64 *)((u32*)virt_addr+i), rp_data, (u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+ONCHIP_MEM_BASE)+i, ep_data); 
		else
		            printk(KERN_DEBUG "%p: 0x%08x != %p: 0x%08x => Data mismatch", (u64 *)((u32*)virt_addr+i), rp_data, (u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+OFFCHIP_MEM_BASE)+i, ep_data);
			    printk(KERN_DEBUG "ep_tmp = %08x\n", ep_tmp);

	    printk(KERN_DEBUG "MIS-MATCH at offset %08x\n", i);
		for(j = 0; j < 300; j++){
			ep_tmp = ioread32((u32 *)(bk_ptr->bar[BAR]+mem_byte_offset+OFFCHIP_MEM_BASE)+i+j);
			rp_data = *((u32*)virt_addr+i+j);
			printk(KERN_DEBUG "RP offset %08x = %08x, EP = %08x\n", i+j, rp_data, ep_tmp);
		}
            return 1;
        } else {
            //printk(KERN_DEBUG "%p: 0x%08x == %p: 0x%08x", (u64 *)((u32*)virt_addr+i), rp_data, (u32 *)(bk_ptr->bar[0]+mem_byte_offset+ONCHIP_MEM_BASE)+i, ep_data); 
        }
    }
    return 0;
}

static int init_ep_mem(struct altera_pcie_dma_bookkeep *bk_ptr, u32 mem_byte_offset, u32 num_dwords, u32 init_value, u8 increment)
{
    u32 i = 0;
    u32 increment_value = 0;
    u32 tmp_rand;
    for (i = 0; i < num_dwords; i++) {
        if (increment) increment_value = i;  
 //       iowrite32 (cpu_to_le32(init_value+increment_value), (u32 *)(bk_ptr->bar[BAR]+mem_byte_offset)+increment_value);
	get_random_bytes(&tmp_rand, sizeof(tmp_rand));
        iowrite32 (cpu_to_le32(tmp_rand), (u32 *)(bk_ptr->bar[BAR]+mem_byte_offset)+i);
	wmb();
	}

    return 0;
}

static int set_lite_table_header(struct lite_dma_header *header)
{
    int i;
    for (i = 0; i < 128; i++)
        header->flags[i] = cpu_to_le32(0x0); 
    return 0;
}

static int dma_test(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev)
{

    u8 *rp_rd_buffer_virt_addr = bk_ptr->rp_rd_buffer_virt_addr;
    dma_addr_t rp_rd_buffer_bus_addr = bk_ptr->rp_rd_buffer_bus_addr;
    u8 *rp_wr_buffer_virt_addr = bk_ptr->rp_wr_buffer_virt_addr;
    dma_addr_t rp_wr_buffer_bus_addr = bk_ptr->rp_wr_buffer_bus_addr;
    int loop_count = 0, num_loop_count = 1, simul_read_count, simul_write_count;
    int i, j;
u32 rp_tmp, ep_tmp;
    u32 last_id, write_127;
    u32	timeout;
    u32 r_last_id, w_last_id, r_write_127, w_write_127;
    u32 rand;
    
    struct timespec64 tv1;
    struct timespec64 tv2;
    struct timespec64 diff;
    atomic_set(&bk_ptr->status, 1);
    bk_ptr->dma_status.pass_read = 0;     
    bk_ptr->dma_status.pass_write = 0;     
    bk_ptr->dma_status.pass_simul = 0;   

    if(bk_ptr->dma_status.rand){
	get_random_bytes(&rand, sizeof(rand));
	rand = rand & 0x3FFFF;
    	if(rand == 0) rand = 1;
    	bk_ptr->dma_status.altera_dma_num_dwords = rand;
    
    	get_random_bytes(&rand, sizeof(rand));
    	rand = rand & 0x7F;
    	if(rand == 0) rand = 1;
    	bk_ptr->dma_status.altera_dma_descriptor_num = rand;
    }

    memset(rp_rd_buffer_virt_addr, 0, bk_ptr->dma_status.altera_dma_num_dwords*4);
    memset(rp_wr_buffer_virt_addr, 0, bk_ptr->dma_status.altera_dma_num_dwords*4);
    init_rp_mem(rp_rd_buffer_virt_addr, bk_ptr->dma_status.altera_dma_num_dwords, 0x00000000, 1);
    init_rp_mem(rp_wr_buffer_virt_addr, bk_ptr->dma_status.altera_dma_num_dwords, 0x00000000, 1);

    bk_ptr->dma_status.read_eplast_timeout = 0;
    bk_ptr->dma_status.write_eplast_timeout = 0;
    
	if (bk_ptr -> dma_status.onchip)
		init_ep_mem(bk_ptr, (u64)ONCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, 0x0, 1);
	else
		init_ep_mem(bk_ptr, (u64)OFFCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, 0x0, 1);


    if(bk_ptr->dma_status.run_read) {

	timeout = TIMEOUT;

	write_127 = 0;

	last_id = ioread32((u32 *)(bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR));

	set_lite_table_header((struct lite_dma_header *)bk_ptr->lite_table_rd_cpu_virt_addr);
        wmb();
        for (i = 0; i < 128/*bk_ptr->dma_status.altera_dma_descriptor_num*/; i++) {
		if(bk_ptr->dma_status.onchip)
	            set_read_desc(&bk_ptr->lite_table_rd_cpu_virt_addr->descriptors[i], (dma_addr_t)rp_rd_buffer_bus_addr, (u64)ONCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, i);
		else
	            set_read_desc(&bk_ptr->lite_table_rd_cpu_virt_addr->descriptors[i], (dma_addr_t)rp_rd_buffer_bus_addr, (u64)OFFCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, i);
}
        iowrite32 ((dma_addr_t)bk_ptr->lite_table_rd_bus_addr, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_RC_LOW_SRC_ADDR);
        iowrite32 (((dma_addr_t)bk_ptr->lite_table_rd_bus_addr)>>32, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_RC_HIGH_SRC_ADDR);
	
	if(last_id == 0xFF){
	        iowrite32 (RD_CTRL_BUF_BASE_LOW, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_CTLR_LOW_DEST_ADDR);
        	iowrite32 (RD_CTRL_BUF_BASE_HI, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_CTRL_HIGH_DEST_ADDR);
	}
        wmb();

	if(last_id == 0xFF) last_id = 127;
	
	last_id = last_id + bk_ptr->dma_status.altera_dma_descriptor_num;

	if(last_id > 127){
		last_id = last_id - 128;
		if((bk_ptr->dma_status.altera_dma_descriptor_num > 1) && (last_id != 127)) write_127 = 1;
	}

	ktime_get_real_ts64(&tv1);
	
	if(write_127) iowrite32 (127, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR);

	iowrite32 (last_id, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR);
	
	while (1) {
                    if (bk_ptr->lite_table_rd_cpu_virt_addr->header.flags[last_id]) {
                        break;
                    }
		    
		    if(timeout == 0){
			printk(KERN_DEBUG "Read DMA times out\n");
			bk_ptr->dma_status.read_eplast_timeout = 1;
			printk(KERN_DEBUG "DWORD = %08x\n", bk_ptr->dma_status.altera_dma_num_dwords);
    			printk(KERN_DEBUG "Desc = %08x\n", bk_ptr->dma_status.altera_dma_descriptor_num);
			break;
		    }

		    timeout--;
                    cpu_relax();
	    }

        ktime_get_real_ts64(&tv2);  
        diff_timeval(&diff, &tv2, &tv1);
        bk_ptr->dma_status.read_time = diff; 

	if(timeout == 0){
		bk_ptr->dma_status.pass_read = 0;
	}
	else{
		if (rp_ep_compare(rp_rd_buffer_virt_addr, bk_ptr, 0, bk_ptr->dma_status.altera_dma_num_dwords)) {
        	    bk_ptr->dma_status.pass_read = 0;
        	}
        	else
        	    bk_ptr->dma_status.pass_read = 1;
    		}
	}

    if (bk_ptr->dma_status.run_write) {
	timeout = TIMEOUT;
	write_127 = 0;
	last_id = ioread32((u32 *)(bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR));
	//printk(KERN_DEBUG "Read ID = %08x\n", last_id);

        memset(rp_wr_buffer_virt_addr, 0, bk_ptr->dma_status.altera_dma_num_dwords*4);
	
        set_lite_table_header((struct lite_dma_header *)bk_ptr->lite_table_wr_cpu_virt_addr);
        wmb();
        for (i = 0; i < 128/*bk_ptr->dma_status.altera_dma_descriptor_num*/; i++) {
		if(bk_ptr->dma_status.onchip)      
		    set_write_desc(&bk_ptr->lite_table_wr_cpu_virt_addr->descriptors[i], ONCHIP_MEM_BASE, (dma_addr_t)rp_wr_buffer_bus_addr, bk_ptr->dma_status.altera_dma_num_dwords, i);
		else
			set_write_desc(&bk_ptr->lite_table_wr_cpu_virt_addr->descriptors[i], OFFCHIP_MEM_BASE, (dma_addr_t)rp_wr_buffer_bus_addr, bk_ptr->dma_status.altera_dma_num_dwords, i);
        }

        iowrite32 ((dma_addr_t)bk_ptr->lite_table_wr_bus_addr, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_RC_LOW_SRC_ADDR);
        iowrite32 (((dma_addr_t)bk_ptr->lite_table_wr_bus_addr)>>32, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_RC_HIGH_SRC_ADDR);
	if(last_id == 0xFF){        
		iowrite32 (WR_CTRL_BUF_BASE_LOW, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_CTLR_LOW_DEST_ADDR);
        	iowrite32 (WR_CTRL_BUF_BASE_HI, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_CTRL_HIGH_DEST_ADDR);
	}

        wmb();
        if(last_id == 0xFF) last_id = 127;
	
	last_id = last_id + bk_ptr->dma_status.altera_dma_descriptor_num;

	if(last_id > 127){
		last_id = last_id - 128;
		if((bk_ptr->dma_status.altera_dma_descriptor_num > 1) && (last_id != 127)) write_127 = 1;
	}
	
	ktime_get_real_ts64(&tv1);  	
	
	if(write_127) iowrite32 (127, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR);

	iowrite32 (last_id, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR);

	//printk(KERN_DEBUG "write_127 = %08x\n", write_127);
	//printk(KERN_DEBUG "Write ID = %08x\n", last_id);
		
	while (1) {
                    if (bk_ptr->lite_table_wr_cpu_virt_addr->header.flags[last_id]) {
                        break;
                    }
		    
		    if(timeout == 0){
			bk_ptr->dma_status.write_eplast_timeout = 1;
			printk(KERN_DEBUG "Write DMA times out\n");
			printk(KERN_DEBUG "DWORD = %08x\n", bk_ptr->dma_status.altera_dma_num_dwords);
			printk(KERN_DEBUG "Desc = %08x\n", bk_ptr->dma_status.altera_dma_descriptor_num);
			break;
		    }

		    timeout--;
                    cpu_relax();
	    }
	
        ktime_get_real_ts64(&tv2);  
        diff_timeval(&diff, &tv2, &tv1);
        bk_ptr->dma_status.write_time = diff;

	if(timeout == 0){
		bk_ptr->dma_status.pass_write = 0;
	}
	else{
	        if (rp_ep_compare(rp_wr_buffer_virt_addr, bk_ptr, 0, bk_ptr->dma_status.altera_dma_num_dwords)) {
        	    bk_ptr->dma_status.pass_write = 0;
        	}
        	else
        	    bk_ptr->dma_status.pass_write = 1;   
    	}
    }

    if(bk_ptr->dma_status.run_simul) {
	timeout = TIMEOUT;
	r_write_127 = 0;
	w_write_127 = 0;
	r_last_id = ioread32((u32 *)(bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR));
	w_last_id = ioread32((u32 *)(bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR));

        set_lite_table_header((struct lite_dma_header *)bk_ptr->lite_table_rd_cpu_virt_addr);
        set_lite_table_header((struct lite_dma_header *)bk_ptr->lite_table_wr_cpu_virt_addr);
        memset(rp_rd_buffer_virt_addr, 0, bk_ptr->dma_status.altera_dma_num_dwords*4);
        memset(rp_wr_buffer_virt_addr, 0, bk_ptr->dma_status.altera_dma_num_dwords*4);
        init_rp_mem(rp_rd_buffer_virt_addr, bk_ptr->dma_status.altera_dma_num_dwords, 0x00000000, 1);
	if(bk_ptr->dma_status.onchip)
	        init_ep_mem(bk_ptr, /*bk_ptr->dma_status.altera_dma_num_dwords*4 + */(u64)ONCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, 0x0, 1);
	else
	        init_ep_mem(bk_ptr, /*bk_ptr->dma_status.altera_dma_num_dwords*4 + */(u64)OFFCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, 0x0, 1);
        wmb();

        for (i = 0; i < 128/*bk_ptr->dma_status.altera_dma_descriptor_num*/; i++) {
		if(bk_ptr->dma_status.onchip)
	            set_read_desc(&bk_ptr->lite_table_rd_cpu_virt_addr->descriptors[i], (dma_addr_t)rp_rd_buffer_bus_addr, ONCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, i);
		else
		    set_read_desc(&bk_ptr->lite_table_rd_cpu_virt_addr->descriptors[i], (dma_addr_t)rp_rd_buffer_bus_addr, OFFCHIP_MEM_BASE, bk_ptr->dma_status.altera_dma_num_dwords, i);
        }

        for (i = 0; i < 128/*bk_ptr->dma_status.altera_dma_descriptor_num*/; i++) {
		if(bk_ptr->dma_status.onchip)
			set_write_desc(&bk_ptr->lite_table_wr_cpu_virt_addr->descriptors[i], ONCHIP_MEM_BASE+4*bk_ptr->dma_status.altera_dma_num_dwords, (dma_addr_t)(rp_wr_buffer_bus_addr), bk_ptr->dma_status.altera_dma_num_dwords, i);
		else
	               set_write_desc(&bk_ptr->lite_table_wr_cpu_virt_addr->descriptors[i], OFFCHIP_MEM_BASE+4*bk_ptr->dma_status.altera_dma_num_dwords, (dma_addr_t)(rp_wr_buffer_bus_addr), bk_ptr->dma_status.altera_dma_num_dwords, i);
		
        }
	//Program source read descriptor table lower 32-bit in RC into register thru bar0
        iowrite32 ((dma_addr_t)bk_ptr->lite_table_rd_bus_addr, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_RC_LOW_SRC_ADDR);

	//Program source read descriptor table upper 32-bit in RC into register thru bar0
        iowrite32 (((dma_addr_t)bk_ptr->lite_table_rd_bus_addr)>>32, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_RC_HIGH_SRC_ADDR);

	if(r_last_id == 0xFF){
		//Program destination read descriptor table lower 32-bit in FPGA into register thru bar0
        	iowrite32 (RD_CTRL_BUF_BASE_LOW, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_CTLR_LOW_DEST_ADDR);

		//Program destination read descriptor table upper 32-bit in FPGA into register thru bar0
        	iowrite32 (WR_CTRL_BUF_BASE_HI, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_CTRL_HIGH_DEST_ADDR);
	}

	//Program source write descriptor table lower 32-bit in RC into register thru bar0
        iowrite32 ((dma_addr_t)bk_ptr->lite_table_wr_bus_addr, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_RC_LOW_SRC_ADDR);

	//Program source write descriptor table upper 32-bit in RC into register thru bar0
        iowrite32 (((dma_addr_t)bk_ptr->lite_table_wr_bus_addr)>>32, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_RC_HIGH_SRC_ADDR);

	if(w_last_id == 0xFF){
		//Program destination write descriptor table lower 32-bit in FPGA into register thru bar0
        	iowrite32 (WR_CTRL_BUF_BASE_LOW, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_CTLR_LOW_DEST_ADDR);

		//Program destination write descriptor table upper 32-bit in FPGA into register thru bar0
        	iowrite32 (WR_CTRL_BUF_BASE_HI, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_CTRL_HIGH_DEST_ADDR);
	}
        wmb();

	if(r_last_id == 0xFF) r_last_id = 127;
	if(w_last_id == 0xFF) w_last_id = 127;
	
	r_last_id = r_last_id + bk_ptr->dma_status.altera_dma_descriptor_num;
	w_last_id = w_last_id + bk_ptr->dma_status.altera_dma_descriptor_num;

	if(r_last_id > 127){
		r_last_id = r_last_id - 128;
		if((bk_ptr->dma_status.altera_dma_descriptor_num > 1) && (r_last_id != 127)) r_write_127 = 1;
	}

	if(w_last_id > 127){
		w_last_id = w_last_id - 128;
		if((bk_ptr->dma_status.altera_dma_descriptor_num > 1) && (w_last_id != 127)) w_write_127 = 1;
	}

	//Get start time        
	ktime_get_real_ts64(&tv1);  

	if(r_write_127) iowrite32 (127, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR);
	iowrite32 (r_last_id, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_RD_LAST_PTR);

	if(w_write_127) iowrite32 (127, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR);
	iowrite32 (w_last_id, bk_ptr->bar[0]+DESC_CTRLLER_BASE+ALTERA_LITE_DMA_WR_LAST_PTR);
		
	while (1) {
                    if ((bk_ptr->lite_table_wr_cpu_virt_addr->header.flags[w_last_id]) & (bk_ptr->lite_table_rd_cpu_virt_addr->header.flags[r_last_id])) 		    {
                        break;
                    }
		    
		    if(timeout == 0){
			bk_ptr->dma_status.read_eplast_timeout = 1;
			bk_ptr->dma_status.write_eplast_timeout = 1;
			printk(KERN_DEBUG "Simultaneous DMA times out\n");
			printk(KERN_DEBUG "DWORD = %08x\n", bk_ptr->dma_status.altera_dma_num_dwords);
			printk(KERN_DEBUG "Desc = %08x\n", bk_ptr->dma_status.altera_dma_descriptor_num);
			break;
		    }

		    timeout--;
                    cpu_relax();
	    }
	
        ktime_get_real_ts64(&tv2);  
        diff_timeval(&diff, &tv2, &tv1);
        bk_ptr->dma_status.simul_time = diff;
	if(timeout == 0){
		bk_ptr->dma_status.pass_simul = 0;
	}
	else{
	       if (rp_ep_compare((u8 *)rp_rd_buffer_virt_addr, bk_ptr, 0, bk_ptr->dma_status.altera_dma_num_dwords) || rp_ep_compare((u8 *)rp_wr_buffer_virt_addr, bk_ptr, bk_ptr->dma_status.altera_dma_num_dwords*4, bk_ptr->dma_status.altera_dma_num_dwords)) {
        	    bk_ptr->dma_status.pass_simul = 0;
	        }     
        	else{
        	  bk_ptr->dma_status.pass_simul = 1;
		}
    	}
	}
	
    atomic_set(&bk_ptr->status, 0);
    wake_up(&bk_ptr->wait_q);
    return 0;

}

static int __init map_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev)
{
    int i;
    for (i = 0; i < ALTERA_DMA_BAR_NUM; i++) {
        unsigned long bar_start = pci_resource_start(dev, i);
        //unsigned long bar_end = pci_resource_end(dev, i);
        //unsigned long bar_flags = pci_resource_flags(dev, i);
        bk_ptr->bar_length[i] = pci_resource_len(dev, i);
        if (!bk_ptr->bar_length[i]) {
            bk_ptr->bar[i] = NULL;
            continue;
        }
        bk_ptr->bar[i] = ioremap(bar_start, bk_ptr->bar_length[i]);
        if (!bk_ptr->bar[i]) {
            dev_err(&dev->dev, "could not map BAR[%d]", i);
            return -1;
        } else
            dev_info(&dev->dev, "BAR[%d] mapped to 0x%p, length %lu", i, bk_ptr->bar[i], (long unsigned int)bk_ptr->bar_length[i]); 
    }
    return 0;
}

static void unmap_bars(struct altera_pcie_dma_bookkeep *bk_ptr, struct pci_dev *dev)
{
    int i;
    for (i = 0; i < ALTERA_DMA_BAR_NUM; i++) {
        if (bk_ptr->bar[i]) {
            pci_iounmap(dev, bk_ptr->bar[i]);
            bk_ptr->bar[i] = NULL;
        }
    }
}

static inline void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size, dma_addr_t *dma_handle)
{
    return dma_alloc_coherent(hwdev == NULL ? NULL : &hwdev->dev, size, dma_handle, GFP_ATOMIC);
}

static inline void pci_free_consistent(struct pci_dev *hwdev, size_t size, void *vaddr, dma_addr_t dma_handle)
{
    dma_free_coherent(hwdev == NULL ? NULL : &hwdev->dev, size, vaddr, dma_handle);
}

static int __init altera_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    int rc = 0;
    struct altera_pcie_dma_bookkeep *bk_ptr = NULL;

    bk_ptr = kzalloc(sizeof(struct altera_pcie_dma_bookkeep), GFP_KERNEL);
    if(!bk_ptr)
        goto err_bk_alloc;

    bk_ptr->pci_dev = dev;
    pci_set_drvdata(dev, bk_ptr);

    rc = init_chrdev(bk_ptr); 
    if (rc) {
        dev_err(&dev->dev, "init_chrdev() failed\n");
        goto err_initchrdev;
    }
    rc = pci_enable_device(dev);
    if (rc) {
        dev_err(&dev->dev, "pci_enable_device() failed\n");
        goto err_enable;
    } else {
        dev_info(&dev->dev, "pci_enable_device() successful");
    }
    rc = pci_request_regions(dev, ALTERA_DMA_DRIVER_NAME);
    if (rc) {
        dev_err(&dev->dev, "pci_request_regions() failed\n");
        goto err_regions;
    }
    pci_set_master(dev);
    rc = pci_enable_msi(dev);
    if (rc) {
        dev_info(&dev->dev, "pci_enable_msi() failed\n");
        bk_ptr->msi_enabled = 0;
    } else {
        dev_info(&dev->dev, "pci_enable_msi() successful\n");
        bk_ptr->msi_enabled = 1;
    }
    pci_read_config_byte(dev, PCI_REVISION_ID, &bk_ptr->revision);
    pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &bk_ptr->irq_pin);
    pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &bk_ptr->irq_line);

    if (!dma_set_mask_and_coherent(&dev->dev, DMA_BIT_MASK(DMAMASK))) {
    // if (!pci_set_dma_mask(dev, DMA_BIT_MASK(DMAMASK))) {
        dev_info(&dev->dev, "using a 64-bit irq mask\n");
    } else {
        dev_info(&dev->dev, "unable to use 64-bit irq mask\n");
        goto err_dma_mask;
    }

    dev_info(&dev->dev, "irq pin: %d\n", bk_ptr->irq_pin);
    dev_info(&dev->dev, "irq line: %d\n", bk_ptr->irq_line);
    dev_info(&dev->dev, "irq: %d\n", dev->irq);

    rc = 0;
//request_irq(bk_ptr->irq_line, dma_isr, IRQF_SHARED, ALTERA_DMA_DRIVER_NAME, (void *)bk_ptr); 

    if (rc) {
        dev_info(&dev->dev, "Could not request IRQ #%d", bk_ptr->irq_line);
        bk_ptr->irq_line = -1;
        goto err_irq;
    } else {
        dev_info(&dev->dev, "request irq: %d", bk_ptr->irq_line);
    }

    scan_bars(bk_ptr, dev);
    map_bars(bk_ptr, dev);

    // waitqueue for user process
    init_waitqueue_head(&bk_ptr->wait_q);

    // set default settings to run
    bk_ptr->dma_status.altera_dma_num_dwords = ALTERA_DMA_NUM_DWORDS;
    bk_ptr->dma_status.altera_dma_descriptor_num = ALTERA_DMA_DESCRIPTOR_NUM;
    bk_ptr->dma_status.run_write = 1;
    bk_ptr->dma_status.run_read = 1;
    bk_ptr->dma_status.run_simul = 1;
    bk_ptr->dma_status.offset = 0;
    bk_ptr->dma_status.onchip = 1;
    bk_ptr->dma_status.rand = 0;
    bk_ptr->table_rd_cpu_virt_addr = ((struct dma_desc_table *)pci_alloc_consistent(dev, sizeof(struct dma_desc_table), &bk_ptr->table_rd_bus_addr));
    bk_ptr->lite_table_rd_cpu_virt_addr = ((struct lite_dma_desc_table *)pci_alloc_consistent(dev, sizeof(struct lite_dma_desc_table), &bk_ptr->lite_table_rd_bus_addr));
    if (!bk_ptr->table_rd_cpu_virt_addr || !bk_ptr->lite_table_rd_cpu_virt_addr) {
        rc = -ENOMEM;
        goto err_rd_table;
    }
    bk_ptr->table_wr_cpu_virt_addr = ((struct dma_desc_table *)pci_alloc_consistent(dev, sizeof(struct dma_desc_table), &bk_ptr->table_wr_bus_addr));
    bk_ptr->lite_table_wr_cpu_virt_addr = ((struct lite_dma_desc_table *)pci_alloc_consistent(dev, sizeof(struct lite_dma_desc_table), &bk_ptr->lite_table_wr_bus_addr));
    if (!bk_ptr->table_wr_cpu_virt_addr || !bk_ptr->lite_table_wr_cpu_virt_addr) {
        rc = -ENOMEM;
        goto err_wr_table;
    }
    bk_ptr->numpages = (PAGE_SIZE >= MAX_NUM_DWORDS*4) ? 1 : (int)((MAX_NUM_DWORDS*4)/PAGE_SIZE);
    bk_ptr->rp_rd_buffer_virt_addr = pci_alloc_consistent(dev, PAGE_SIZE*bk_ptr->numpages, &bk_ptr->rp_rd_buffer_bus_addr);
    if (!bk_ptr->rp_rd_buffer_virt_addr) {
        rc = -ENOMEM;
        goto err_rd_buffer;
    }
    bk_ptr->rp_wr_buffer_virt_addr = pci_alloc_consistent(dev, PAGE_SIZE*bk_ptr->numpages, &bk_ptr->rp_wr_buffer_bus_addr);
    if (!bk_ptr->rp_wr_buffer_virt_addr) {
        rc = -ENOMEM;
        goto err_wr_buffer;
    }
    return 0;

    // error clean up
err_wr_buffer:
    dev_err(&dev->dev, "goto err_wr_buffer");
    pci_free_consistent(dev, PAGE_SIZE*bk_ptr->numpages, bk_ptr->rp_rd_buffer_virt_addr, bk_ptr->rp_rd_buffer_bus_addr);
err_rd_buffer:
    dev_err(&dev->dev, "goto err_rd_buffer");
    pci_free_consistent(dev, sizeof(struct dma_desc_table), bk_ptr->table_wr_cpu_virt_addr, bk_ptr->table_wr_bus_addr);
err_wr_table:
    dev_err(&dev->dev, "goto err_wr_table");
    pci_free_consistent(dev, sizeof(struct dma_desc_table), bk_ptr->table_rd_cpu_virt_addr, bk_ptr->table_rd_bus_addr);
err_rd_table:
    dev_err(&dev->dev, "goto err_rd_table");
err_irq:
    dev_err(&dev->dev, "goto err_regions");
err_dma_mask:
    dev_err(&dev->dev, "goto err_dma_mask");
    pci_release_regions(dev);
err_regions:
    dev_err(&dev->dev, "goto err_irq");
    pci_disable_device(dev);
err_enable:
    dev_err(&dev->dev, "goto err_enable");
    unregister_chrdev_region (bk_ptr->cdevno, 1);
err_initchrdev:
    dev_err(&dev->dev, "goto err_initchrdev");
    kfree(bk_ptr);
err_bk_alloc:
    dev_err(&dev->dev, "goto err_bk_alloc");
    return rc;
}


static void __exit altera_pci_remove(struct pci_dev *dev)
{
    struct altera_pcie_dma_bookkeep *bk_ptr = NULL;
    bk_ptr = pci_get_drvdata(dev);
    cdev_del(&bk_ptr->cdev);
    unregister_chrdev_region(bk_ptr->cdevno, 1);
    pci_disable_device(dev);
    if(bk_ptr) {
        if(bk_ptr->msi_enabled) {
            pci_disable_msi(dev);
            bk_ptr->msi_enabled = 0;
        }
    }
    unmap_bars(bk_ptr, dev);
    pci_release_regions(dev);
    if (bk_ptr->irq_line >= 0) {
        printk(KERN_DEBUG "Freeing IRQ #%d", bk_ptr->irq_line);
        free_irq(bk_ptr->irq_line, (void *)bk_ptr);
    }
    pci_free_consistent(dev, sizeof(struct dma_desc_table), bk_ptr->table_rd_cpu_virt_addr, bk_ptr->table_rd_bus_addr);
    pci_free_consistent(dev, sizeof(struct lite_dma_desc_table), bk_ptr->lite_table_rd_cpu_virt_addr, bk_ptr->lite_table_rd_bus_addr);
    pci_free_consistent(dev, sizeof(struct dma_desc_table), bk_ptr->table_wr_cpu_virt_addr, bk_ptr->table_wr_bus_addr);
    pci_free_consistent(dev, sizeof(struct lite_dma_desc_table), bk_ptr->lite_table_wr_cpu_virt_addr, bk_ptr->lite_table_wr_bus_addr);
    pci_free_consistent(dev, PAGE_SIZE*bk_ptr->numpages, bk_ptr->rp_rd_buffer_virt_addr, bk_ptr->rp_rd_buffer_bus_addr);
    pci_free_consistent(dev, PAGE_SIZE*bk_ptr->numpages, bk_ptr->rp_wr_buffer_virt_addr, bk_ptr->rp_wr_buffer_bus_addr);

    kfree(bk_ptr);
    printk(KERN_DEBUG ALTERA_DMA_DRIVER_NAME ": " "altera_dma_remove()," " " __DATE__ " " __TIME__ " " "\n");
}

static struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(ALTERA_DMA_VID, ALTERA_DMA_DID) },
    { 0 }
};

static struct pci_driver dma_driver_ops = {
    .name = ALTERA_DMA_DRIVER_NAME,
    .id_table = pci_ids,
    .probe = altera_pci_probe,
    .remove = altera_pci_remove,
};

static int __init altera_dma_init(void)
{
    int rc = 0;

    printk(KERN_DEBUG ALTERA_DMA_DRIVER_NAME ": " "altera_dma_init()," " " __DATE__ " " __TIME__ " " "\n");
    rc = pci_register_driver(&dma_driver_ops);
    if (rc) {
        printk(KERN_ERR ALTERA_DMA_DRIVER_NAME ": PCI driver registration failed\n");
        goto exit;
    }

exit:
    return rc;
}

static void __exit altera_dma_exit(void)
{
    pci_unregister_driver(&dma_driver_ops);
}
/*
static int eplast_busy_wait(struct altera_pcie_dma_bookkeep *bk_ptr, u32 expected_eplast, u8 rw)
{
    // rw: 1 = read, 0 = write
    u32 timeout = 0;
    u32 eplast = 0;
    while (1) {
        eplast = (rw == 1? *(u32*)bk_ptr->table_rd_cpu_virt_addr: *(u32*)bk_ptr->table_wr_cpu_virt_addr);
        if (eplast == expected_eplast)
            break; 
        ++timeout;
        if (timeout == TIMEOUT_THRESH) {
            printk(KERN_DEBUG "Timed out waiting for EPLAST");
            if (rw == 1)
                bk_ptr->dma_status.read_eplast_timeout = 1;
            else
                bk_ptr->dma_status.write_eplast_timeout = 1;
            return -1;
        }
        udelay(1);
    }
    if (rw == 1)
        bk_ptr->dma_status.read_eplast_timeout = 0;
    else
        bk_ptr->dma_status.write_eplast_timeout = 0;
    return 0;
}
*/


module_init(altera_dma_init);
module_exit(altera_dma_exit);

MODULE_AUTHOR("Michael Chen <micchen@altera.com>");
MODULE_DESCRIPTION("256b DMA Driver");
MODULE_VERSION(ALTERA_DMA_DRIVER_VERSION);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DEVICE_TABLE(pci, pci_ids);


