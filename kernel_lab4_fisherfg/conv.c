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
    for (int i = 0; i < kernel_len; i++){
        CONV_BASE[CONV_KERNEL_OFFSET] = kernel_array[i];
    }
    for (int i = 0; i < CONV_ELEMENT_LEN - 1; i++){
        CONV_BASE[CONV_DATA_OFFSET] = (uint64_t)0;
        uint64_t nul;
        nul = CONV_BASE[CONV_RESULT_HI_OFFSET];
        nul = CONV_BASE[CONV_RESULT_LO_OFFSET];
    }
    for (int i = 0; i < data_len; i++){
        CONV_BASE[CONV_DATA_OFFSET] = data_array[i];
        dest[i<<1] = CONV_BASE[CONV_RESULT_HI_OFFSET];
        dest[(i<<1)+1] = CONV_BASE[CONV_RESULT_LO_OFFSET];
    }
    for (int i = 0; i < CONV_ELEMENT_LEN - 1; i++){
        CONV_BASE[CONV_DATA_OFFSET] = (uint64_t)0;
        dest[(i+data_len)<<1] = CONV_BASE[CONV_RESULT_HI_OFFSET];
        dest[(i+data_len)<<1+1] = CONV_BASE[CONV_RESULT_LO_OFFSET];
    }
    return;
}


void mul_uint64(const uint64_t a, const uint64_t b, uint64_t* result_hi, uint64_t* result_lo){
    *result_hi = 0;
    *result_lo = 0;
    for (int i = 0; i < 64; ++i) {
        if (b & (1ULL << i)) { 
            uint64_t shifted_a_lo = a << i; 
            uint64_t shifted_a_hi = ( i == 0 ? 0 : a >> (64 - i));
            uint64_t temp_lo = *result_lo + shifted_a_lo; 
            uint64_t temp_hi = *result_hi + shifted_a_hi; 
            if (temp_lo < *result_lo) { 
                temp_hi += 1ULL; 
            }
            *result_lo = temp_lo;
            *result_hi = temp_hi;
        }
    }
}

void mul_compute(const uint64_t* data_array, size_t data_len, const uint64_t* kernel_array, size_t kernel_len, uint64_t* dest){
    for (int i = 0; i < data_len + CONV_ELEMENT_LEN - 1; i++) {
        uint64_t temp_lo[CONV_ELEMENT_LEN];
        uint64_t temp_hi[CONV_ELEMENT_LEN];
        uint64_t sum_lo = 0;
        uint64_t sum_hi = 0;
        for (int j = 0; j < CONV_ELEMENT_LEN; j++) {
            uint64_t hi, lo;
            uint64_t data;
            if ((i + j < 3) | (i + j > 18)){
                data = 0;
            } else {
                data = data_array[i + j - 3];
            }
            mul_uint64(kernel_array[j], data, &hi, &lo);
            sum_lo += lo;
            sum_hi += hi;
            if (sum_lo < lo) {
                sum_hi += 1;
            }
        }
        dest[(i<<1)] = sum_hi;
        dest[(i<<1)+1] = sum_lo;
    }
}
