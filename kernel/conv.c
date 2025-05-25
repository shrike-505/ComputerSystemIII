#include "conv.h"
typedef unsigned long long int size_t;
uint64_t* CONV_BASE = (uint64_t*)0x10001000L;
const size_t CONV_KERNEL_OFFSET = 0;
const size_t CONV_DATA_OFFSET = 1;
const size_t CONV_RESULT_LO_OFFSET = 0;
const size_t CONV_RESULT_HI_OFFSET = 1;
const size_t CONV_STATE_OFFSET = 2;
const unsigned char READY_MASK = 0b01;
const size_t CONV_ELEMENT_LEN = 4;

uint64_t* MISC_BASE = (uint64_t*)0x10002000L;
const size_t MISC_TIME_OFFSET = 0;

uint64_t get_time(void){
    return MISC_BASE[MISC_TIME_OFFSET];
}

void conv_compute(const uint64_t* data_array, size_t data_len, const uint64_t* kernel_array, size_t kernel_len, uint64_t* dest){
    for(size_t i = 0; i < kernel_len; i++){
        CONV_BASE[CONV_KERNEL_OFFSET] = kernel_array[i];
    }
    for(size_t i = 0; i < kernel_len-1; i++){
        CONV_BASE[CONV_DATA_OFFSET] = 0;
    }
    size_t index =0;
    for(size_t i = 0; i < data_len; i++){
        CONV_BASE[CONV_DATA_OFFSET] = data_array[i];
        while(!(CONV_BASE[CONV_STATE_OFFSET] & READY_MASK));
        dest[index+1] = CONV_BASE[CONV_RESULT_LO_OFFSET];
        dest[index] = CONV_BASE[CONV_RESULT_HI_OFFSET];
        
        index+=2;
    }

    for(size_t i = 0; i < kernel_len - 1; i++){
        CONV_BASE[CONV_DATA_OFFSET] = 0;
        while(!(CONV_BASE[CONV_STATE_OFFSET] & READY_MASK));
        dest[index+1] = CONV_BASE[CONV_RESULT_LO_OFFSET];
        dest[index] = CONV_BASE[CONV_RESULT_HI_OFFSET];
        index+=2;
    }

}

// int BigAdder128(uint64_t a_hi, uint64_t a_lo, uint64_t b_hi, uint64_t b_lo, uint64_t* result_hi, uint64_t* result_lo){
//     int carry = 0;
//     *result_lo = a_lo + b_lo;
//     if(*result_lo < a_lo|| *result_lo < b_lo){
//         carry = 1;
//     }
//     *result_hi = a_hi + b_hi + carry;
//     return carry;
// }
// void BigMul64(uint64_t a, uint64_t b, uint64_t* result_hi, uint64_t* result_lo){
//     *result_hi = 0;
//     *result_lo = 0;
//     for(size_t i = 0; i < 64; i++){
//         if((a >> i) & 1){
//             *result_lo += b << i;
//             if(i!=0)
//                 *result_hi += b >> (64 - i);
//             if(*result_lo < b << i){
//                 *(result_hi)++;
//             }
//         }
//     }
// }
   
void mul_compute(const uint64_t* data_array, size_t data_len, const uint64_t* kernel_array, size_t kernel_len, uint64_t* dest){
    for (size_t i = 0; i < 2 * (data_len + kernel_len - 1); i++) {
        dest[i] = 0;
    }
    // uint64_t buff;
    // for (size_t i = 0; i < kernel_len-1; i++){
    //     buff[i] = 0;
    // }
    // for (size_t i = kernel_len-1; i < data_len+kernel_len-1; i++){
    //     buff[i] = data_array[i-(kernel_len-1)];
    // }
    // for (size_t i = data_len+kernel_len-1; i < 2*(kernel_len-1)+data_len; i++){
    //     buff[i] = 0;
    // }

    for (size_t i = 0; i < data_len+kernel_len-1;i++){
        uint64_t temp[2] = {0, 0};
        uint64_t sum[2] = {0, 0};
        uint64_t buff;
        // if(i < kernel_len-1){
        //     buff = 0;
        // }else if(i < data_len+kernel_len-1){
        //     buff = data_array[i-(kernel_len-1)];
        // }else{
        //     buff = 0;
        // }
        for (size_t j = 0; j < kernel_len; j++){
            if(i+j < kernel_len-1){
                buff = 0;
            }else if(i+j < data_len+kernel_len-1){
                buff = data_array[i+j-(kernel_len-1)];
            }else{
                buff = 0;
            }
            temp[0] = 0;
            temp[1] = 0;
            for (size_t k = 0; k < 64; k++){
                if((kernel_array[j] >> k) & 1){
                    temp[0] += buff << k;
                    if(k!=0)
                        temp[1] += buff >> (64 - k);
                    if(temp[0] < buff << k){
                        temp[1]++;
                    }
                }
            }
            int carry = 0;
            uint64_t ro = sum[0];
            sum[0] = sum[0] + temp[0];
            if(sum[0] < temp[0] || sum[0] < ro){
                carry = 1;
            }
            sum[1] = sum[1] + temp[1] + carry;
        }
        dest[2*i] = sum[1];
        dest[2*i+1] = sum[0];
    }
}