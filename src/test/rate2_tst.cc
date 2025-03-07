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
#include <unistd.h>

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
    // uint64_t            txClkSum=0;
    // uint32_t            txClkMax=0, txClkMin=0xffffffff;
    // uint32_t            txClkMaxGlobal=0, txClkMinGlobal=0xffffffff;
    // float               bw, bwMin=1.0/0.0, bwMax=-1.0/0.0, bwSum=0.0;
float               bw;
    // float               bwMinGlobal=1.0/0.0, bwMaxGlobal=-1.0/0.0;
    char const *        bwHistogramFile = "/tmp/bwHistogram.data";
    uint64_t            bwHistogram[HISTOGRAM_SIZE] = {0};
    // uint64_t            numPackagesTotal = 0;
    uint32_t            i;
    // clock_t             t0;
    struct sched_param  param;
    Hub                 *root = static_cast<Hub*>(arg);

    
    printf("Rx thread started\n");
    
    Path       pre      = IPath::create       (*root);
    Stream     strm     = IStream::create     (pre->findByName("/Stream0"));
    ScalVal    swClear  = IScalVal::create    (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareClear"));
    ScalVal_RO txClkCnt = IScalVal_RO::create (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareBwidthCnt"));
    ScalVal    swEnable = IScalVal::create    (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable"));


    swClear->setVal((uint64_t)1);
    swClear->setVal((uint64_t)0);

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
    swEnable->setVal((uint64_t)1);
    int cnt=0;
    // t0 = clock();
    while(run)
    {
        got = strm->read(buf, sizeof(buf), CTimeout(20000000));
	cnt++;
        if ( ! got )
        { 
            fprintf(stderr,"RX thread timed out\n");
        }
        else
        {
        	// Number of FPGA clock it took the current TX
        	txClkCnt->getVal(&u32);

        	// Current RX bandwidth
            bw = (float)(got)/float(u32) * FPGA_CLK_FREQ/1000000;

            // Update histogram
            bwHistogram[int(round(bw))] += 1;
         //    numPackagesTotal += 1;

        	// // Bandwidth statistics
        	// bwSum += bw;
			
        	// if (bw > bwMax)
        	// 	bwMax = bw;
			
        	// if (bw < bwMin)
        	// 	bwMin = bw;

        	// // FPGA clock statistics
        	// txClkSum += u32;
   
        	// if (u32 < txClkMin)
        	// 	txClkMin = u32;
   
        	// if (u32 > txClkMax)
        	// 	txClkMax = u32;

         //    // Global clock statistics
         //    if (u32 < txClkMinGlobal)
         //        txClkMinGlobal = u32;
   
         //    if (u32 > txClkMaxGlobal)
         //        txClkMaxGlobal = u32;

         //    if (bw > bwMaxGlobal)
         //        bwMaxGlobal = bw;
            
         //    if (bw < bwMinGlobal)
         //        bwMinGlobal = bw;

        	// Update number of point taken for the nex average
        	AvePointCnt++;

            if (AvePointCnt >= 360)
        	{
        		// printf("\n");
        		// printf("CPU ticks:         %f\n",             (double)(clock()-t0));
          //       printf("Packages received: %"PRIu16"\n",      AvePointCnt);
        		// printf("Max RX time:       %f ms\n",          (float)(txClkMax) * 1000/FPGA_CLK_FREQ);
        		// printf("Min RX time:       %f ms\n",          (float)(txClkMin) * 1000/FPGA_CLK_FREQ);
        		// printf("Average RX time    %f ms\n",          (float)(txClkSum) / AvePointCnt * 1000/FPGA_CLK_FREQ);
        		// printf("Max RX Rate:       %f MB/s\n",        bwMax);
        		// printf("Min RX Rate:       %f MB/s\n",        bwMin);
        		// printf("Average RX Rate:   %f MB/s\n",        (bwSum / AvePointCnt));
	
        		// txClkSum = 0;
        		// txClkMin = 0xffffffff;
        		// txClkMax = 0;
        		// bwSum = 0.0;
        		// bwMin = 1.0/0.0;
        		// bwMax = -1.0/0.0;
        		AvePointCnt = 0;
        		// t0 = clock();
        	}
        }

    }

    // printf("\n");
    // printf("Exiting RX thread...\n");

    // printf("\n");
    // printf("Disabling software...\n");
    // swEnable->setVal((uint64_t)0);

    // printf("\n");
    // printf("Max RX time during test:  %f ms\n",     (float)(txClkMaxGlobal) * 1000/FPGA_CLK_FREQ);
    // printf("Min RX time during test:  %f ms\n",     (float)(txClkMinGlobal) * 1000/FPGA_CLK_FREQ);
    // printf("Max RX Rate during test:  %f MB/s\n",   bwMaxGlobal);
    // printf("Min RX Rate during test:  %f MB/s\n",   bwMinGlobal);

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
    root = IPath::loadYamlFile("./CentralNodeYAML/000TopLevel.yaml", "NetIODev")->origin();

    Path       pre         = IPath::create       (root );
    ScalVal_RO fpgaVers    = IScalVal_RO::create (pre->findByName("/mmio/AmcCarrierCore/AxiVersion/FpgaVersion"));
    ScalVal_RO bldStamp    = IScalVal_RO::create (pre->findByName("/mmio/AmcCarrierCore/AxiVersion/BuildStamp"));
    ScalVal_RO gitHash     = IScalVal_RO::create (pre->findByName("/mmio/AmcCarrierCore/AxiVersion/GitHash"));
    ScalVal    heartBeat   = IScalVal::create    (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdHeartbeat"));
    ScalVal    enable      = IScalVal::create    (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable"));
    ScalVal_RO swLossError = IScalVal_RO::create (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossError"));
    ScalVal_RO swLossCnt   = IScalVal_RO::create (pre->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossCnt"));

    printf("\n");
    printf("AxiVersion:\n");
    fpgaVers->getVal(&u64);
    printf("FpgaVersion: %"PRIu64"\n", u64);
    bldStamp->getVal(str, sizeof(str)/sizeof(str[0]));
    printf("Build String:\n%s\n", (char*)str);
    gitHash->getVal(hash, 21);
    printf("Git Hash:\n0x");
    for (int i = 0; i < 20; i++)
        printf("%X", hash[i]);
    printf("\n");

    pthread_t tid;
    if ( pthread_create(&tid, 0, rxThread, &root )) 
        perror("pthread_create");

    printf("\n");
    printf("Enabling the message stream...\n");
    enable->setVal((uint64_t)1);

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
    enable->setVal((uint64_t)0);

    pthread_join(tid, NULL);

    printf("\n");
    swLossError->getVal(&u8);
    printf("Software packet loss error: %"PRIu8"\n", u8);
    swLossCnt->getVal(&u32);
    printf("Number of packets loss:     %"PRIu32"\n", u32);
    /*
    Children rootsChildren = pre->origin()->getChildren();
    std::vector<Child>::const_iterator it;
    
    for ( it = rootsChildren->begin(); it != rootsChildren->end(); ++it ) {
      (*it)->dump();
    }
    */

    return 0;
}
