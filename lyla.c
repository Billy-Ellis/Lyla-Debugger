//
//
// Lyla Debugger for ARM iOS
// devices
//
// Developed by Billy Ellis 
// @bellis1000
//
//
//
// you'll need the following entitlements, and to be running as root
/*
 
 <?xml version="1.0" encoding="UTF-8"?>
 <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd …">
 <plist version="1.0">
 <dict>
 <key>get-task-allow</key>
 <true/>
 <key>proc_info-allow</key>
 <true/>
 <key>run-unsigned-code</key>
 <true/>
 <key>task_for_pid-allow</key>
 <true/>
 </dict>
 </plist>
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <mach/mach_types.h>
#include <mach/vm_map.h>
#include <mach/mach_host.h>
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/types.h>

int v; // verbose mode flag

void registers(mach_port_t port){

    thread_act_port_array_t thread_list;
    mach_msg_type_number_t thread_count;
    
    // get threads in task
    task_threads(port, &thread_list, &thread_count);

    if (v){
        printf("[\033[1m\x1b[35mv\x1b[0m] Number of threads in process: %x\n",thread_count);
    }
    
    arm_thread_state_t arm_state;
    mach_msg_type_number_t sc = ARM_THREAD_STATE_COUNT;
    
    // get register state from first thread
    thread_get_state(thread_list[0],ARM_THREAD_STATE,(thread_state_t)&arm_state,&sc);
    
    printf("\n\033[1mRegister dump:\x1b[0m\n\n    \033[1mR0:\x1b[0m 0x%.08x    \033[1mR1:\x1b[0m 0x%.08x\n    \033[1mR2:\x1b[0m 0x%.08x    \033[1mR3:\x1b[0m 0x%.08x\n    \033[1mR4:\x1b[0m 0x%.08x    \033[1mR5:\x1b[0m 0x%.08x\n    \033[1mR6:\x1b[0m 0x%.08x    \033[1mR7:\x1b[0m 0x%.08x\n    \033[1mR8:\x1b[0m 0x%.08x    \033[1mR9:\x1b[0m 0x%.08x\n    \033[1mR10:\x1b[0m 0x%.08x   \033[1mR11:\x1b[0m 0x%.08x\n    \033[1mR12:\x1b[0m 0x%.08x   \033[1mLR:\x1b[0m 0x%.08x\n    \033[1mSP:\x1b[0m 0x%.08x    \033[1mPC:\x1b[0m 0x%.08x\n    \033[1mCPSR:\x1b[0m 0x%.08x\n\n",arm_state.__r[0],arm_state.__r[1],arm_state.__r[2],arm_state.__r[3],arm_state.__r[4],arm_state.__r[5],arm_state.__r[6],arm_state.__r[7],arm_state.__r[8],arm_state.__r[9],arm_state.__r[10],arm_state.__r[11],arm_state.__r[12],arm_state.__lr,arm_state.__sp,arm_state.__pc,arm_state.__cpsr);
}

void read_from(uint32_t addr, size_t size, mach_port_t port){
    
    unsigned char buf[size];
    kern_return_t kr = vm_read_overwrite(port,(vm_address_t)addr,size,(vm_address_t)&buf,&size);

    if (kr != KERN_SUCCESS){
        printf("[!] Read failed. %s\n",mach_error_string(kr));
    }else{
        if (v){
            printf("[\033[1m\x1b[35mv\x1b[0m] Call to vm_read_overwrite() succeeded.\n");
        }
        int a = 0;
        for (int i = 0; i < size; i+=8){
            printf("0x%.08x: %.02x%.02x%.02x%.02x %.02x%.02x%.02x%.02x\n",addr+i,buf[a],buf[a+1],buf[a+2],buf[a+3],buf[a+4],buf[a+5],buf[a+6],buf[a+7]);
            a+=8;
        }
    }
}

void write_what_where(uint32_t addr, uint32_t data, mach_port_t port){

    kern_return_t kr = vm_write(port,(vm_address_t)addr,(vm_address_t)&data,sizeof(data));
    
    if (kr != KERN_SUCCESS){
        printf("[!] Write failed. %s\n",mach_error_string(kr));
    }else{
        if (v){
            printf("[\033[1m\x1b[35mv\x1b[0m] Call to vm_write() succeeded.\n");
        }
        printf("Wrote bytes.\n");
    }
}

void regset(char reg[], uint32_t value, mach_port_t port){
    
    thread_act_port_array_t thread_list;
    mach_msg_type_number_t thread_count;
    
    // get threads in task
    task_threads(port, &thread_list, &thread_count);
    
    if (v){
        printf("[\033[1m\x1b[35mv\x1b[0m] Number of threads in process: %x\n",thread_count);
    }
    
    arm_thread_state_t arm_state;
    mach_msg_type_number_t sc = ARM_THREAD_STATE_COUNT;
    
    // get register state from first thread
    thread_get_state(thread_list[0],ARM_THREAD_STATE,(thread_state_t)&arm_state,&sc);

    for(int i=0;i<16;i++){
        char destreg[20];
        if(i<13){
            sprintf(destreg,"R%d",i);
            if (strcmp(reg,destreg)==0){
                arm_state.__r[i] = value;
            }
        } else if(i==13){
            if (strcmp(reg,"SP")==0){
                arm_state.__sp = value;
            }
        } else if(i==14){
            if (strcmp(reg,"LR")==0){
                arm_state.__lr = value;
            }
        } else if(i==15){
            if (strcmp(reg,"PC")==0){
                arm_state.__pc = value;
            }
        } else{
            printf("[!] Invalid register name specified.\n");
        }
    }
    
//    if (strcmp(reg,"R0")==0){
//        arm_state.__r[0] = value;
//    }else if (strcmp(reg,"R1")==0){
//        arm_state.__r[1] = value;
//    }else if (strcmp(reg,"R2")==0){
//        arm_state.__r[2] = value;
//    }else if (strcmp(reg,"R3")==0){
//        arm_state.__r[3] = value;
//    }else if (strcmp(reg,"R4")==0){
//        arm_state.__r[4] = value;
//    }else if (strcmp(reg,"R5")==0){
//        arm_state.__r[5] = value;
//    }else if (strcmp(reg,"R6")==0){
//        arm_state.__r[6] = value;
//    }else if (strcmp(reg,"R7")==0){
//        arm_state.__r[7] = value;
//    }else if (strcmp(reg,"R8")==0){
//        arm_state.__r[8] = value;
//    }else if (strcmp(reg,"R9")==0){
//        arm_state.__r[9] = value;
//    }else if (strcmp(reg,"R10")==0){
//        arm_state.__r[10] = value;
//    }else if (strcmp(reg,"R11")==0){
//        arm_state.__r[11] = value;
//    }else if (strcmp(reg,"R12")==0){
//        arm_state.__r[12] = value;
//    }else if (strcmp(reg,"SP")==0){
//        arm_state.__sp = value;
//    }else if (strcmp(reg,"LR")==0){
//        arm_state.__lr = value;
//    }else if (strcmp(reg,"PC")==0){
//        arm_state.__pc = value;
//    }else{
//        printf("[!] Invalid register name specified.\n");
//    }
    
    thread_set_state(thread_list[0],ARM_THREAD_STATE,(thread_state_t)&arm_state,sc);
    
    printf("Registers updated.\n");
}

void cli(mach_port_t port){

    while (1){

        char input[64];
        
        printf("\033[1m<lyla> \x1b[0m");
        scanf(" %63[^\n]",input);

        // split input into individual words (to determine command, arguments etc)
        char cmd[5][20]={0};
        int i, j, k;
        j=0;
        k=0;
        for(i=0; i < strlen(input); i++){
            if(input[i] == ' '){
                if(input[i+1] != ' '){
                    cmd[k][j]='\0';
                    j=0;
                    k++;
                }
                continue;
            }
            else{
                //copy other characters
                cmd[k][j++] = input[i];
            }
        }
        cmd[k][j]='\0';
        //

        if (strcmp(cmd[0],"help")==0){

            printf("\n\033[1m   help\x1b[0m  - prints this help message\n\033[1m   read <address> <size>\x1b[0m  -  reads specified amount of bytes from specified address\n\033[1m   write <data> <address>\x1b[0m  -  writes specified data to specified address\n\033[1m   registers\x1b[0m  -  displays the current state of the registers in the program being debugged\n\033[1m   regset <register> <value>\x1b[0m  -  displays the current state of the registers in the program being debugged\n\033[1m   suspend\x1b[0m  -  suspends the current process being debugged\n\033[1m   resume\x1b[0m  -  resumes execution the current process being debugged\n\033[1m   q\x1b[0m  -  quit Lyla\n\n");

        }else if (strcmp(cmd[0],"q")==0){
            exit(0);
        }else if (strcmp(cmd[0], "registers")==0){
            registers(port);
        }else if (strcmp(cmd[0], "regset")==0){
            uint32_t value = (int)strtol(cmd[2],NULL,16);
            regset(cmd[1],value,port);
        }else if (strcmp(cmd[0], "read")==0){
            uint32_t addr = (int)strtol(cmd[1],NULL,16);
            size_t size = (int)strtol(cmd[2],NULL,16);
            read_from(addr,size,port);
        }else if (strcmp(cmd[0], "write")==0){
            uint32_t addr = (int)strtol(cmd[1],NULL,16);
            uint32_t data = (int)strtol(cmd[2],NULL,16);
            write_what_where(addr,data,port);
        }else if (strcmp(cmd[0],"suspend")==0){
            task_suspend(port);
            printf("Task suspended.\n");
        }else if (strcmp(cmd[0],"resume")==0){
            task_resume(port);
            printf("Task resumed.\n");
        }else{
            printf("[!] Invalid command.\n");
        }
    }
}

void check_root(){
    // make sure Lyla is running as root
    if (getuid() && geteuid()){
        printf("Lyla isn't running as root.\nCannot continue.\n");
        exit(0);
    }
}

int main(int argc, char *argv[]){

    v = 0;
    if (argc >= 2){
        if (strcmp(argv[1],"-v")==0){
            // verbose mode enabled
            v = 1;
            printf("[\033[1m\x1b[35mv\x1b[0m] Verbose mode enabled.\n");
        }
    }

    printf("\n\033[1mLyla Debugger for \x1b[32mARM\x1b[0m\033[1m\nVersion:1.0\nDeveloped by @bellis1000\n\n\x1b[0m");
    check_root();

    printf("Enter PID to attach to >> ");
    int pid;
    scanf("%d",&pid);
    printf("\x1b[0m");
    
    if (pid == 0){
        printf("Lyla does not currently support kernel debugging.\n");
        exit(0);
    }

    printf("Attaching to PID %d...\n\n",pid);

    mach_port_t port;
    kern_return_t kr;

    if ((kr = task_for_pid(mach_task_self(), pid, &port)) != KERN_SUCCESS){
        printf("[!] Error!\n");
        if (v){
            printf("[\033[1m\x1b[35mv\x1b[0m] Call to task_for_pid() with PID %d failed.\n",pid);
        }
        exit(0);
    }

    if (v){
        printf("\033[1m[\x1b[35mv\x1b[0m] Got task port 0x%x for PID %d\n",port,pid);
    }

    printf("Attached PID %d\n\n",pid);

    cli(port);

    return 0;
    
}
