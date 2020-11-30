/*
* Copyright (c) 2017 - 2020, Mohammed Elbadry
*
*
* This file is part of V-MAC (Pub/Sub data-centric Multicast MAC layer)
*
* V-MAC is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 
* 4.0 International License.
* 
* You should have received a copy of the license along with this
* work. If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
* 
*/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include "vmac-usrsp.h"
/**
 * DOC: Introdcution
 * This document defines a code to implement VMAC sender or receiver
 */

/**
 * DOC : Using VMAC sender
 * This receiver is a standard C executable. Simply compile and run
 * Eg: gcc stress-test.c vmac.a -pthread -lm
 *
 *  ./a.out -p  --> p signfies this is the sender/producer
 */

/**
 * DOC : Using VMAC receiver
 * This receiver is a standard C executable. Simply compile and run
 * Eg: gcc stress-test.c vmac.a -pthread -lm
 *
 *  ./a.out -c  --> c signfies this is the receiver/consumer
 */

/**
 * DOC : Warning
 * In a standard test run, always run the sender BEFORE
 * you run the receiver  as the sender waits for an interest from the receiver.
 * Do not use both -p and -c arguments while running the code. Use either one
 */

/**
 *
 * DOC :Frame format
 * The sender sends 1024 byte Ethernet  frames with only sequence number inserted as data.
 *
 */

pthread_t thread;
pthread_t sendth;
pthread_t appth;
volatile int running2=0;
volatile int total;
volatile int consumer=0;
volatile int producer=0;
int times=0;
FILE *sptr,*cptr,*fptr;
double loss=0.0;
int c;
struct tm *loc;
unsigned int count =0;
long ms;
time_t s;
struct timespec spec;
int window[1500];
double intTime;

/**
 * vmac_send_interest  - Sends interest packet
 *
 * Creates an interest frame and sends  it. Run as a C thread process
 *
 * Arguments : @tid : Thread id which is a automatically created when calling
 * pthread_create. Do NOT set this manually
 *
 * @param      tid   thread ID.
 *
 * @return     void
 */
void *vmac_send_interest(void* tid)
{
    int i;
    char* dataname="chat";
    uint16_t name_len=strlen(dataname);
    char buffer[93]="buffer";
    total=0;
    struct vmac_frame frame;
    struct meta_data meta;

    while(1)
    {
        total = 0;
        frame.buf = buffer;
        frame.len = 93;
        frame.InterestName = dataname;
        frame.name_len = 4;
        meta.type = VMAC_FC_INT;
        meta.rate = 6.5;
        send_vmac(&frame, &meta);
        clock_gettime(CLOCK_REALTIME,&spec);
        s=spec.tv_sec;
        ms=round(spec.tv_nsec / 1.0e6);
        intTime = spec.tv_sec;
        intTime += spec.tv_nsec /1.0e9;
        if (ms > 999) 
        {
            s++;
            ms=0;
        }
        printf("==============================================================================\n========================================NEW RUN================================\n================================================================================");
        printf("Sent @ timestamp=%lu %"PRIdMAX".%03ld\n",(unsigned long)time(NULL),(intmax_t)s, ms);
        sleep(100);
        
    }
}

/**
 *  vmac_send_data - VMAC producer
 *
 *  Creates data frames and sends them to receiver(s). Run as a C thread process
 *
 *  Arguments :
 *  @tid : Thread id which is a automatically created when calling pthread_create. In this case
 *  not run as thread. Default value of 0 to be used
 */

void *vmac_send_data(void* tid)
{
    
    char* dataname="chat";
    char sel='a';
    char msgy[1024];
    int i = 0, j = 0;
    uint16_t name_len = strlen(dataname);
    uint16_t len = 1023;  
    struct vmac_frame frame;
    struct meta_data meta;
    running2 = 1;
    printf("Sleeping for 15 seconds\n");
    sleep(15);
    printf("Sending no.%d\n", times++);
    memset(msgy,sel,1023);
    msgy[1023] = '\0';
    meta.type = VMAC_FC_DATA;
    meta.rate = 60.0;
    frame.buf = msgy;
    frame.len = len;
    frame.InterestName = dataname;
    frame.name_len = 4;
    for(i = 0; i < 50000; i++)
    {
        meta.seq = i;

        send_vmac(&frame, &meta);
    }
    running2 = 0;
}

/**
 * recv_frame - VMAC recv frame function
 *
 * @param      frame  struct containing frame buffer, interestname (if available), and their lengths respectively.
 * @param      meta   The meta meta information about frame currently: type, seq, encoding, and rate,
 */
void callbacktest(struct vmac_frame *frame, struct meta_data *meta)
{
    uint8_t type = meta->type; 
    uint64_t enc = meta->enc;
    double goodput;
    char* buff = frame->buf;
    uint16_t len = frame->len;
    uint16_t seq = meta->seq;
    double frameSize = 0.008928; /* in megabits  1116 bytes after V-MAC and 802.11 headers*/
    uint16_t interestNameLen = frame->name_len;
    double timediff;
    double waittime = 15; /* 15 seconds waiting/sleep for all interests to come in */
    clock_gettime(CLOCK_REALTIME,&spec);
    s = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6);
    if (ms > 999) 
    {
        s++;
        ms = 0;
    }
    timediff = spec.tv_sec;
    timediff += spec.tv_nsec / 1.0e9;
    timediff = timediff - intTime;
    if (type == VMAC_FC_INT && producer == 1 && running2 == 0)
    {
        pthread_create(&sendth, NULL, vmac_send_data, (void*)0);
        printf("type:%u and seq=%d and count=%u @%"PRIdMAX".%03ld\n", type, seq, count, (intmax_t)s, ms);
    }
    else if (type == VMAC_FC_DATA && consumer)
    {
        total++;
        loss = ((double)(50000 - total) / 50000) * (double) 100;
        goodput = (double)(total * frameSize) / (timediff - waittime);
        printf("type:%u | seq=%d | loss=%f | goodput=%f |T= %f\n", type, seq, loss, goodput, timediff - waittime);
        printf("content= %s \n length =%d\n", frame->buf, frame->len);
    }
    free(frame);
    free(meta);
}

/**
 *  run_vmac  - Decides if sender or receiver.
 *
 *  Decides if VMAC sender of receiver
 *
 *  Arguments :
 *  @weare: 0 - Sender, 1 - Receiver
 *
 */
void run_vmac(int weare)
{

    uint8_t type;
    uint16_t len,name_len;
    uint8_t flags=0;
    pthread_t consumerth;
    char dataname[1600];
    char choice;

    choice = weare;
    if (choice == 0)
    {
        printf("We are producer\n");    
        producer = 1;
    }
    else if (choice == 1)
    {
        printf("We are consumer\n");
        running2 = 1;
        producer = 0;
        consumer = 1;
        pthread_create(&consumerth,NULL,vmac_send_interest,(void*)0);
    }
}

/**
 * main - Main function
 *
 * Function registers, calls run_vmac
 *
 * Arguments
 * p or c (Look at DOC Using VMAC sender or VMAC receiver
 */

int  main(int argc, char *argv[]){
    int weare=0;
    void (*ptr)()=&callbacktest;
    vmac_register(ptr);
    if(argc < 2 ) 
    { 
        return -1;
    }

    if (strncmp(argv[1], "p", sizeof(argv[1])) == 0) 
    {
        weare = 0;
    } 
    else  
    {
        weare=1;
    } 

    run_vmac(weare);
    while (1) 
    { 
        sleep(1);
    }
    return 1 ;  
}
