#ifndef FIBDRV_BIG_NUM_H_
#define FIBDRV_BIG_NUM_H_

typedef struct BigNum {
    unsigned long long lower;
    unsigned long long upper;
} __attribute__((packed)) BigNum;



#endif /* FIBDRV_BIG_NUM_H_ */
