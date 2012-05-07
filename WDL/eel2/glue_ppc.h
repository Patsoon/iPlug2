#ifndef _NSEEL_GLUE_PPC_H_
#define _NSEEL_GLUE_PPC_H_

#define GLUE_MAX_JMPSIZE 30000 // maximum relative jump size for this arch (if not defined, any jump is possible)
#define GLUE_JMP_TYPE short
#define GLUE_JMP_OFFSET (-4) // jumps are from start of instruction on ppc

static const unsigned char GLUE_JMP_NC[] = { 0x48,0, 0, 0, }; // b <offset>

static const unsigned int GLUE_JMP_IF_P1_Z[]=
{
  0x2f830000,  //cmpwi cr7, r3, 0
  0x419e0000,  // beq cr7, offset-bytes-from-startofthisinstruction
};
static const unsigned int GLUE_JMP_IF_P1_NZ[]=
{
  0x2f830000,  //cmpwi cr7, r3, 0
  0x409e0000,  // bne cr7, offset-bytes-from-startofthisinstruction
};


#define GLUE_MOV_PX_DIRECTVALUE_SIZE 8
static void GLUE_MOV_PX_DIRECTVALUE_GEN(void *b, INT_PTR v, int wv) 
{   
  static const unsigned short tab[3][2] = {
    {0x3C60, 0x6063}, // addis r3, r0, hw -- ori r3,r3, lw
    {0x3DC0, 0x61CE}, // addis r14, r0, hw -- ori r14, r14, lw
    {0x3DE0, 0x61EF}, // addis r15, r0, hw -- oris r15, r15, lw
  };
  unsigned int uv=(unsigned int)v;
	unsigned short *p=(unsigned short *)b;

  *p++ = tab[wv][0]; // addis rX, r0, hw
	*p++ = (uv>>16)&0xffff;
  *p++ = tab[wv][1]; // ori rX, rX, lw
	*p++ = uv&0xffff;
}


// mflr r5
// stwu r5, -8(r1)
const static unsigned int GLUE_FUNC_ENTER[2] = { 0x7CA802A6, 0x94A1FFF8 };

// lwz r5, 0(r1)
// addi r1, r1, 8
// mtlr r5
const static unsigned int GLUE_FUNC_LEAVE[3] = { 0x80A10000, 0x38210008, 0x7CA803A6 };
#define GLUE_FUNC_ENTER_SIZE sizeof(GLUE_FUNC_ENTER)
#define GLUE_FUNC_LEAVE_SIZE sizeof(GLUE_FUNC_LEAVE)

const static unsigned int GLUE_RET[]={0x4E800020}; // blr

static int GLUE_RESET_WTP(unsigned char *out, void *ptr)
{
  const static unsigned int GLUE_SET_WTP_FROM_R17=0x7E308B78; // mr r16 (dest), r17 (src)
  if (out) memcpy(out,&GLUE_SET_WTP_FROM_R17,sizeof(GLUE_SET_WTP_FROM_R17));
  return sizeof(GLUE_SET_WTP_FROM_R17);

}



// stwu r3, -8(r1)
const static unsigned int GLUE_PUSH_P1[1]={ 0x9461FFF8};


#define GLUE_POP_PX_SIZE 8
static void GLUE_POP_PX(void *b, int wv)
{
  static const unsigned int tab[3] ={
      0x80610000, // lwz r3, 0(r1)
      0x81c10000, // lwz r14, 0(r1)
      0x81e10000, // lwz r15, 0(r1)
  };
  ((unsigned int *)b)[0] = tab[wv];
  ((unsigned int *)b)[1] = 0x38210008; // addi r1,r1, 8
}

#define GLUE_SET_PX_FROM_P1_SIZE 4
static void GLUE_SET_PX_FROM_P1(void *b, int wv)
{
  static const unsigned int tab[3]={
    0x7c631b78, // never used: mr r3, r3
    0x7c6e1b78, // mr r14, r3
    0x7c6f1b78, // mr r15, r3
  };
  *(unsigned int *)b  = tab[wv];
}



// lfd f2, 0(r3)
// stfdu f2, -8(r1)
static const unsigned int GLUE_PUSH_P1PTR_AS_VALUE[] = { 0xC8430000, 0xDC41FFF8 };

static int GLUE_POP_VALUE_TO_ADDR(unsigned char *buf, void *destptr)
{    
  // lfd f2, 0(r1)
  // addi r1,r1,8
  // GLUE_MOV_PX_DIRECTVALUE_GEN / GLUE_MOV_PX_DIRECTVALUE_SIZE (r3)
  // stfd f2, 0(r3)
  if (buf)
  {
    unsigned int *bufptr = (unsigned int *)buf;
    *bufptr++ = 0xC8410000;
    *bufptr++ = 0x38210008;    
    GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
    bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;
    *bufptr++ = 0xd8430000;
  }
  return 2*4 + GLUE_MOV_PX_DIRECTVALUE_SIZE + 4;
}

static int GLUE_COPY_VALUE_AT_P1_TO_PTR(unsigned char *buf, void *destptr)
{    
  // lfd f2, 0(r3)
  // GLUE_MOV_PX_DIRECTVALUE_GEN / GLUE_MOV_PX_DIRECTVALUE_SIZE (r3)
  // stfd f2, 0(r3)

  if (buf)
  {
    unsigned int *bufptr = (unsigned int *)buf;
    *bufptr++ = 0xc8430000;
    GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
    bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;
    *bufptr++ = 0xd8430000;
  }
  
  return 4 + GLUE_MOV_PX_DIRECTVALUE_SIZE + 4;
}



static void GLUE_CALL_CODE(INT_PTR bp, INT_PTR cp) 
{
  __asm__(
          "stmw r14, -80(r1)\n"
          "mtctr %0\n"
          "mr r17, %1\n" 
	  "subi r17, r17, 8\n"
          "mflr r5\n" 
          "stw r5, -84(r1)\n"
          "subi r1, r1, 88\n"
          "bctrl\n"
          "addi r1, r1, 88\n"
          "lwz r5, -84(r1)\n"
          "lmw r14, -80(r1)\n"
          "mtlr r5\n"
            ::"r" (cp), "r" (bp));
};

static unsigned char *EEL_GLUE_set_immediate(void *_p, const void *newv)
{
  // 64 bit ppc would take some work
  unsigned int *p=(unsigned int *)_p;
  while ((p[0]&0x0000FFFF) != 0x0000dead && 
         (p[1]&0x0000FFFF) != 0x0000beef) p++;
  p[0] = (p[0]&0xFFFF0000) | (((unsigned int)newv)>>16);
  p[1] = (p[1]&0xFFFF0000) | (((unsigned int)newv)&0xFFFF);

  return (unsigned char *)(p+1);
}

  #define GLUE_SET_PX_FROM_WTP_SIZE sizeof(int)
  static void GLUE_SET_PX_FROM_WTP(void *b, int wv)
  {
    static const unsigned int tab[3]={
      0x7e038378, // mr r3, r16
      0x7e0e8378, // mr r14, r16
      0x7e0f8378, // mr r15, r16
    };
    *(unsigned int *)b = tab[wv];
  }
  static int GLUE_POP_FPSTACK_TO_PTR(unsigned char *buf, void *destptr)
  {
    // set r3 to destptr
    // stfd f1, 0(r3)
    if (buf)
    {
      unsigned int *bufptr = (unsigned int *)buf;
      GLUE_MOV_PX_DIRECTVALUE_GEN(bufptr, (INT_PTR)destptr,0);
      bufptr += GLUE_MOV_PX_DIRECTVALUE_SIZE/4;

      *bufptr++ = 0xD8230000; // stfd f1, 0(r3)
    }
    return GLUE_MOV_PX_DIRECTVALUE_SIZE + sizeof(int);
  }
  #define GLUE_POP_FPSTACK_SIZE 0
  static const unsigned int GLUE_POP_FPSTACK[1] = { 0 }; // no need to pop, not a stack

  static const unsigned int GLUE_POP_FPSTACK_TOSTACK[] = {
    0xdc21fff8, // stfdu f1, -8(r1)
  };

  static const unsigned int GLUE_POP_FPSTACK_TO_WTP_ANDPUSHADDR[] = { 
    0xdc300008, // stfdu f1, 8(r16)
    0x9601fff8, // stwu r16, -8(r1)
  };

  static const unsigned int GLUE_POP_FPSTACK_TO_WTP[] = { 
    0xdc300008, // stfdu f1, 8(r16)
  };

  #define GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE 4
  static void GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(void *b, int wv)
  {
    static const unsigned int tab[3] = {
      0xC8230000, // lfd f1, 0(r3)
      0xC82E0000, // lfd f1, 0(r14)
      0xC82F0000, // lfd f1, 0(r15)
    };
    *(unsigned int *)b = tab[wv];
  }

#define GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE (sizeof(GLUE_POP_FPSTACK_TO_WTP) + GLUE_SET_PX_FROM_WTP_SIZE)
static void GLUE_POP_FPSTACK_TO_WTP_TO_PX(unsigned char *buf, int wv)
{
  memcpy(buf,GLUE_POP_FPSTACK_TO_WTP,sizeof(GLUE_POP_FPSTACK_TO_WTP));
  GLUE_SET_PX_FROM_WTP(buf + sizeof(GLUE_POP_FPSTACK_TO_WTP),wv); // ppc preincs the WTP, so we do this after
};

static unsigned int GLUE_POP_STACK_TO_FPSTACK[1] = { 0 }; // todo


static const unsigned int GLUE_SET_P1_Z[] =  { 0x38600000 }; // li r3, 0
static const unsigned int GLUE_SET_P1_NZ[] = { 0x38600001 }; // li r3, 1

// end of ppc

#endif
