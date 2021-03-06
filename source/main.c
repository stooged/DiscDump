
#include "ps4.h"


unsigned int long long __readmsr(unsigned long __register) {
	unsigned long __edx;
	unsigned long __eax;
	__asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(__register));
	return (((unsigned int long long)__edx) << 32) | (unsigned int long long)__eax;
}

#define X86_CR0_WP (1 << 16)

static inline __attribute__((always_inline)) uint64_t readCr0(void) {
	uint64_t cr0;
	
	asm volatile (
		"movq %0, %%cr0"
		: "=r" (cr0)
		: : "memory"
 	);
	
	return cr0;
}

static inline __attribute__((always_inline)) void writeCr0(uint64_t cr0) {
	asm volatile (
		"movq %%cr0, %0"
		: : "r" (cr0)
		: "memory"
	);
}

struct auditinfo_addr {
    char useless[184];
};

struct ucred {
	uint32_t useless1;
	uint32_t cr_uid;     // effective user id
	uint32_t cr_ruid;    // real user id
 	uint32_t useless2;
    	uint32_t useless3;
    	uint32_t cr_rgid;    // real group id
    	uint32_t useless4;
    	void *useless5;
    	void *useless6;
    	void *cr_prison;     // jail(2)
    	void *useless7;
    	uint32_t useless8;
    	void *useless9[2];
    	void *useless10;
    	struct auditinfo_addr useless11;
    	uint32_t *cr_groups; // groups
    	uint32_t useless12;
};

struct filedesc {
	void *useless1[3];
    	void *fd_rdir;
    	void *fd_jdir;
};

struct proc {
    	char useless[64];
    	struct ucred *p_ucred;
    	struct filedesc *p_fd;
};

struct thread {
    	void *useless;
    	struct proc *td_proc;
};


int kpayload(struct thread *td){

	struct ucred* cred;
	struct filedesc* fd;

	fd = td->td_proc->p_fd;
	cred = td->td_proc->p_ucred;

	void* kernel_base = &((uint8_t*)__readmsr(0xC0000082))[-0x30EB30];
	uint8_t* kernel_ptr = (uint8_t*)kernel_base;
	void** got_prison0 =   (void**)&kernel_ptr[0xF26010];
	void** got_rootvnode = (void**)&kernel_ptr[0x206D250];

	cred->cr_uid = 0;
	cred->cr_ruid = 0;
	cred->cr_rgid = 0;
	cred->cr_groups[0] = 0;

	cred->cr_prison = *got_prison0;
	fd->fd_rdir = fd->fd_jdir = *got_rootvnode;

	// escalate ucred privs, needed for access to the filesystem ie* mounting & decrypting files
	void *td_ucred = *(void **)(((char *)td) + 304); // p_ucred == td_ucred
	
	// sceSblACMgrIsSystemUcred
	uint64_t *sonyCred = (uint64_t *)(((char *)td_ucred) + 96);
	*sonyCred = 0xffffffffffffffff;
	
	// sceSblACMgrGetDeviceAccessType
	uint64_t *sceProcType = (uint64_t *)(((char *)td_ucred) + 88);
	*sceProcType = 0x3801000000000013; // Max access
	
	// sceSblACMgrHasSceProcessCapability
	uint64_t *sceProcCap = (uint64_t *)(((char *)td_ucred) + 104);
	*sceProcCap = 0xffffffffffffffff; // Sce Process

	// Disable write protection
	uint64_t cr0 = readCr0();
	writeCr0(cr0 & ~X86_CR0_WP);

	// specters debug settings patchs
	*(char *)(kernel_base + 0x2001516) |= 0x14;
	*(char *)(kernel_base + 0x2001539) |= 3;
	*(char *)(kernel_base + 0x200153A) |= 1;
	*(char *)(kernel_base + 0x2001558) |= 1;	

	// debug menu full patches thanks to sealab
	*(uint32_t *)(kernel_base + 0x4CECB7) = 0;
	*(uint32_t *)(kernel_base + 0x4CFB9B) = 0;

	// Target ID Patches :)
	*(uint16_t *)(kernel_base + 0x1FE59E4) = 0x8101;
	*(uint16_t *)(kernel_base + 0X1FE5A2C) = 0x8101;
	*(uint16_t *)(kernel_base + 0x200151C) = 0x8101;

	// enable mmap of all SELF ???
	*(uint8_t*)(kernel_base + 0x31EE40) = 0x90;
	*(uint8_t*)(kernel_base + 0x31EE41) = 0xE9;
	*(uint8_t*)(kernel_base + 0x31EF98) = 0x90;
	*(uint8_t*)(kernel_base + 0x31EF99) = 0x90;

	// Restore write protection
	writeCr0(cr0);

	return 0;
}

double(*ceil)(double x);
int(*sceSysUtilSendSystemNotificationWithText)(int messageType, char* message);

void systemMessage(char* msg) {
	sceSysUtilSendSystemNotificationWithText(222, msg);
}


void copyFile(char *sourcefile, char* destfile)
{
    FILE *src = fopen(sourcefile, "rb");
    if (src)
    {
        FILE *out = fopen(destfile,"wb");
        if (out)
        {
            size_t bytes;
            char *buffer = malloc(65536);
            if (buffer != NULL)
            {
                while (0 < (bytes = fread(buffer, 1, 65536, src)))
                    fwrite(buffer, 1, bytes, out);
                    free(buffer);
            }
            fclose(out);
        }
        else {
        }
        fclose(src);
    }
    else {
    }
}



void copyDir(char *sourcedir, char* destdir)
{
    DIR *dir;
    struct dirent *dp;
    struct stat info;
    char src_path[1024], dst_path[1024];

    dir = opendir(sourcedir);
    if (!dir)
        return;
        mkdir(destdir, 0777);
    while ((dp = readdir(dir)) != NULL)
    {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
        {}
        else
        {
            sprintf(src_path, "%s/%s", sourcedir, dp->d_name);
            sprintf(dst_path, "%s/%s", destdir  , dp->d_name);

            if (!stat(src_path, &info))
            {
                if (S_ISDIR(info.st_mode))
                {
                  copyDir(src_path, dst_path);
                }
                else
                if (S_ISREG(info.st_mode))
                {
                  copyFile(src_path, dst_path);
                }
            }
        }
    }
    closedir(dir);
}







int _main(struct thread *td) {
	initKernel();
	initLibc();

   DIR *dir;
    dir = opendir("/mnt/disc");
    if (!dir)
    {
       syscall(11,kpayload,td);
    }
    else
    {
       closedir(dir);
    }
        int sysUtil = sceKernelLoadStartModule("/system/common/lib/libSceSysUtil.sprx", 0, NULL, 0, 0, 0);
        RESOLVE(sysUtil, sceSysUtilSendSystemNotificationWithText);

        FILE *usbdir = fopen("/mnt/usb0/.dirtest", "wb");
    
         if (!usbdir)
            {
                usbdir = fopen("/mnt/usb1/.dirtest", "wb");
                if (!usbdir)
                {
                        systemMessage("No usb drive found.\nYou must insert a usb drive with about 64gb of free space to dump the disk.");
                        return 0;
                }
                else
                {
                        fclose(usbdir);
                        systemMessage("Dumping disc to USB1");
                        unlink("/mnt/usb1/.dirtest");
                        mkdir("/mnt/usb1/Disc_Dump/", 0777);
                        copyDir("/mnt/disc","/mnt/usb1/Disc_Dump");
                        systemMessage("Dump Complete.");

                }
            }
            else
            {
                        fclose(usbdir);
                        systemMessage("Dumping disc to USB0");
                        unlink("/mnt/usb0/.dirtest");
                        mkdir("/mnt/usb0/Disc_Dump/", 0777);
                        copyDir("/mnt/disc","/mnt/usb0/Disc_Dump");
                        systemMessage("Dump Complete.");
            }


return 0;
}


