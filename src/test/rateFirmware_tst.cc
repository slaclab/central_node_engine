#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <sched.h>
#include <math.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <cpsw_api_user.h>

#include<central_node_firmware.h>

#define FPGA_CLK_FREQ	   (250000000u)	// Hz
#define BW_AVERAGE_S       (1)          // Seconds

#define BW_AVERAGE_CPU_CLK (BW_AVERAGE_S*CLOCKS_PER_SEC)

#define MY_PRIORITY (49) /* we use 49 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

#define BUFFER_SIZE_MAX (100*1024) // 100 kbytes

#define HISTOGRAM_SIZE  (1000)      // Size of the bandwidth histogram array. 
                                    // The histogram is on MB/s from 0 to this value

static volatile int run = 1;

void stack_prefault(void) 
{
    unsigned char dummy[MAX_SAFE_STACK];

    memset(dummy, 0, MAX_SAFE_STACK);
    return;
}

void intHandler(int dummy)
{
    run = 0;
}

static void *rxThread(void *arg)
{
    uint8_t             buf[BUFFER_SIZE_MAX];
    int64_t             got;
    uint32_t            u32;
    uint16_t            AvePointCnt=0;
    float               bw;
    char const *        bwHistogramFile = "/tmp/bwHistogram.data";
    uint64_t            bwHistogram[HISTOGRAM_SIZE] = {0};
    uint32_t            i;
    struct sched_param  param;

    
    printf("Rx thread started\n");
    
    Firmware::getInstance().softwareClear();

    // Declare as real time task
    param.sched_priority = MY_PRIORITY;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) 
    {
        perror("sched_setscheduler failed");
        exit(-1);
    }
        
    // Lock memory
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) 
    {
        perror("mlockall failed");
        exit(-2);
    }

    // Pre-fault our stack
    stack_prefault();

    printf("\n");
    printf("Enabling software...\n");
    Firmware::getInstance().setSoftwareEnable(true);
    int cnt=0;
    // t0 = clock();
    while(run)
    {
      got = Firmware::getInstance().readUpdateStream(buf, sizeof(buf), 20000000);
	cnt++;
        if ( ! got )
        { 
            fprintf(stderr,"RX thread timed out\n");
        }
        else
        {
        	// Number of FPGA clock it took the current TX
	  u32 = Firmware::getInstance().getSoftwareClockCount();

        	// Current RX bandwidth
            bw = (float)(got)/float(u32) * FPGA_CLK_FREQ/1000000;

            // Update histogram
            bwHistogram[int(round(bw))] += 1;

        	// Update number of point taken for the nex average
        	AvePointCnt++;

            if (AvePointCnt >= 360)
        	{
        		AvePointCnt = 0;

        	}
        }

    }

    printf("\n");
    printf("Writing bandwidth histogram to %s... (cnt=%d) ... ", bwHistogramFile, cnt);
    FILE *fp = fopen(bwHistogramFile, "wb");
    fprintf(fp, "# BW(Mb/S)     Counts\n");
    uint64_t aux = 0;
    for (i = 0; i < HISTOGRAM_SIZE; i++)
    {
        aux +=  bwHistogram[i];
        fprintf(fp, "%4d           %d\n", i, bwHistogram[i]);
    }
    fclose(fp);
    printf("done!\n");
    // printf("Total number of packages received: %"PRIu64"\n", numPackagesTotal);

}


int main(int argc, char const *argv[])
{
    Hub         root;
    uint8_t     hb=0;
    uint8_t     str[256];
    uint8_t     u8;
    uint32_t    u32;
    uint64_t    u64;
    uint8_t     hash[21];
    struct sched_param  param;
    signal(SIGINT, intHandler);

    // printf("sysconf(_SC_CLK_TCK) = %lu\nCLOCKS_PER_SEC = %lu\n", sysconf(_SC_CLK_TCK), CLOCKS_PER_SEC);
    
    // Declare as real time task
    param.sched_priority = MY_PRIORITY;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("sched_setscheduler failed");
	exit(-1);
    }
    
    // Lock memory
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        exit(-2);
    }

    // Pre-fault our stack
    stack_prefault();
    
    printf("Loading YAML description...\n");
    Firmware::getInstance().createRoot("./CentralNodeYAML/000TopLevel.yaml");
    Firmware::getInstance().createRegisters();

    std::cout << &(Firmware::getInstance());

    pthread_t tid;
    if ( pthread_create(&tid, 0, rxThread, &root )) 
        perror("pthread_create");

    printf("\n");
    printf("Enabling the message stream...\n");
    Firmware::getInstance().setEnable(true);

    printf("\n");
    printf("Generating heartbeat messages...\n");

    printf("\n");
    printf("Press CTRL+C to stop the program...\n");

    while (run)
    {
        // heartBeat->setVal((uint64_t)(hb^=1));
        usleep(500000);
    }

    printf("\n");
    printf("Heartbeat messages generation stopped\n");

    printf("\n");
    printf("Disabling the message stream...\n");
    Firmware::getInstance().setEnable(false);

    pthread_join(tid, NULL);

    printf("\n");
    printf("Software packet loss error: %"PRIu8"\n", Firmware::getInstance().getSoftwareLossError());
    printf("Number of packets loss:     %"PRIu32"\n", Firmware::getInstance().getSoftwareLossCount());

    return 0;
}
