#include "cpu6502.h"

const uint8_t cpu6502_cycles[256] = {
    7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
    2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
    2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

#define FL(c,f) ((c)->flags & (f))
#define SF(c,f) ((c)->flags |= (f))
#define RF(c,f) ((c)->flags &= ~(f))
#define NZ(c,v) do{RF(c,FLAG_N|FLAG_Z);if((v)&0x80)SF(c,FLAG_N);if(!(v))SF(c,FLAG_Z);}while(0)

void cpu6502_init(cpu6502_t *cpu, uint8_t (*rd)(uint16_t), void (*wr)(uint16_t,uint8_t)) {
    cpu->read=rd; cpu->write=wr; cpu->cycles=0; cpu->irq_pending=false;
    cpu6502_reset(cpu);
}

void cpu6502_reset(cpu6502_t *cpu) {
    cpu->a=0; cpu->x=0; cpu->y=0; cpu->sp=0xFD; cpu->flags=FLAG_I;
    uint8_t lo=cpu->read(0xFFFC), hi=cpu->read(0xFFFD);
    cpu->pc=lo|((uint16_t)hi<<8); cpu->cycles+=7;
}

void cpu6502_nmi(cpu6502_t *cpu) {
    cpu->write(0x100|cpu->sp,(uint8_t)(cpu->pc>>8)); cpu->sp--;
    cpu->write(0x100|cpu->sp,(uint8_t)cpu->pc); cpu->sp--;
    cpu->write(0x100|cpu->sp,cpu->flags&~FLAG_B); cpu->sp--;
    SF(cpu,FLAG_I); uint8_t lo=cpu->read(0xFFFA), hi=cpu->read(0xFFFB);
    cpu->pc=lo|((uint16_t)hi<<8); cpu->cycles+=7;
}

static uint8_t rd(cpu6502_t*c,uint16_t a){return c->read(a);}
static void wr(cpu6502_t*c,uint16_t a,uint8_t v){c->write(a,v);}
static uint16_t r16(cpu6502_t*c,uint16_t a){return rd(c,a)|((uint16_t)rd(c,a+1)<<8);}
static uint16_t r16z(cpu6502_t*c,uint16_t a){return rd(c,a)|((uint16_t)rd(c,(uint8_t)(a+1))<<8);}
static void push(cpu6502_t*c,uint8_t v){wr(c,0x100|c->sp,v);c->sp--;}
static uint8_t pop(cpu6502_t*c){c->sp++;return rd(c,0x100|c->sp);}
static int br(cpu6502_t*c,bool cond){int8_t o=(int8_t)rd(c,c->pc++);if(cond){c->pc+=o;return 1;}return 0;}
static void adc(cpu6502_t*c,uint8_t v){uint16_t t=c->a+v+(FL(c,FLAG_C)?1:0);RF(c,FLAG_C|FLAG_V);if(t>0xFF)SF(c,FLAG_C);if((~(c->a^v)&(c->a^(uint8_t)t))&0x80)SF(c,FLAG_V);NZ(c,c->a=(uint8_t)t);}
static void sbc(cpu6502_t*c,uint8_t v){adc(c,(uint8_t)~v);}
static void cmp(cpu6502_t*c,uint8_t r,uint8_t v){uint8_t t=r-v;RF(c,FLAG_C|FLAG_N|FLAG_Z);if(r>=v)SF(c,FLAG_C);if(!t)SF(c,FLAG_Z);if(t&0x80)SF(c,FLAG_N);}
static void rol_a(cpu6502_t*c){uint8_t oc=FL(c,FLAG_C)?1:0; if(c->a&0x80)SF(c,FLAG_C);else RF(c,FLAG_C);c->a=(c->a<<1)|oc;NZ(c,c->a);}
static void rol_m(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a),oc=FL(c,FLAG_C)?1:0; if(v&0x80)SF(c,FLAG_C);else RF(c,FLAG_C);v=(v<<1)|oc;NZ(c,v);wr(c,a,v);}
static void ror_a(cpu6502_t*c){uint8_t oc=FL(c,FLAG_C)?0x80:0; if(c->a&1)SF(c,FLAG_C);else RF(c,FLAG_C);c->a=(c->a>>1)|oc;NZ(c,c->a);}
static void ror_m(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a),oc=FL(c,FLAG_C)?0x80:0; if(v&1)SF(c,FLAG_C);else RF(c,FLAG_C);v=(v>>1)|oc;NZ(c,v);wr(c,a,v);}
static void asl_m(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a);RF(c,FLAG_C);if(v&0x80)SF(c,FLAG_C);v<<=1;NZ(c,v);wr(c,a,v);}
static void lsr_m(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a);RF(c,FLAG_C);if(v&1)SF(c,FLAG_C);v>>=1;NZ(c,v);wr(c,a,v);}
static void slo(cpu6502_t*c,uint16_t a){asl_m(c,a);c->a|=rd(c,a);NZ(c,c->a);}
static void rla(cpu6502_t*c,uint16_t a){rol_m(c,a);c->a&=rd(c,a);NZ(c,c->a);}
static void sre(cpu6502_t*c,uint16_t a){lsr_m(c,a);c->a^=rd(c,a);NZ(c,c->a);}
static void rra(cpu6502_t*c,uint16_t a){ror_m(c,a);adc(c,rd(c,a));}
static void dcp(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a)-1;NZ(c,v);wr(c,a,v);cmp(c,c->a,v);}
static void isb(cpu6502_t*c,uint16_t a){uint8_t v=rd(c,a)+1;NZ(c,v);wr(c,a,v);sbc(c,v);}

static uint16_t am_idx(cpu6502_t*c){uint8_t z=rd(c,c->pc++);return rd(c,z)|((uint16_t)rd(c,(uint8_t)(z+1))<<8);}
static uint16_t am_idy(cpu6502_t*c){uint8_t z=rd(c,c->pc++);return rd(c,z)|((uint16_t)rd(c,(uint8_t)(z+1))<<8);}

uint32_t cpu6502_step(cpu6502_t*c){
    uint8_t op=rd(c,c->pc++);
    uint32_t cy=cpu6502_cycles[op];
    uint16_t a; uint8_t v;

    switch(op){
    case 0x00: push(c,(uint8_t)((c->pc+1)>>8)); push(c,(uint8_t)(c->pc+1)); push(c,c->flags|FLAG_B); SF(c,FLAG_I); c->pc=r16(c,0xFFFE); break;
    case 0x01: a=r16z(c,rd(c,c->pc++))+c->x;             c->a|=rd(c,a);                     NZ(c,c->a); break;
    case 0x03: a=am_idx(c);                               slo(c,a);                          break;
    case 0x04: rd(c,c->pc++); break;
    case 0x05: a=rd(c,c->pc++);                           c->a|=rd(c,a);                     NZ(c,c->a); break;
    case 0x06: a=rd(c,c->pc++); asl_m(c,a); break;
    case 0x07: a=rd(c,c->pc++); slo(c,a); break;
    case 0x08: push(c,c->flags|FLAG_B); break;
    case 0x09: c->a|=rd(c,c->pc++);                                                         NZ(c,c->a); break;
    case 0x0A: RF(c,FLAG_C); if(c->a&0x80)SF(c,FLAG_C); c->a<<=1;                           NZ(c,c->a); break;
    case 0x0B: c->a&=rd(c,c->pc++); RF(c,FLAG_C); if(c->a&0x80)SF(c,FLAG_C); NZ(c,c->a); break;
    case 0x0C: rd(c,c->pc++); rd(c,c->pc++); break;
    case 0x0D: a=r16(c,c->pc); c->pc+=2;                 c->a|=rd(c,a);                     NZ(c,c->a); break;
    case 0x0E: a=r16(c,c->pc); c->pc+=2; asl_m(c,a); break;
    case 0x0F: a=r16(c,c->pc); c->pc+=2; slo(c,a); break;
    case 0x10: cy+=br(c,!FL(c,FLAG_N)); break;
    case 0x11: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; c->a|=rd(c,a); NZ(c,c->a); break;
    case 0x13: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; slo(c,a); break;
    case 0x14: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0x15: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    c->a|=rd(c,a); NZ(c,c->a); break;
    case 0x16: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; asl_m(c,a); break;
    case 0x17: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; slo(c,a); break;
    case 0x18: RF(c,FLAG_C); break;
    case 0x19: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->a|=rd(c,a); NZ(c,c->a); break;
    case 0x1A: break;
    case 0x1B: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; slo(c,a); break;
    case 0x1C: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0x1D: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; c->a|=rd(c,a); NZ(c,c->a); break;
    case 0x1E: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; asl_m(c,a); break;
    case 0x1F: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; slo(c,a); break;
    case 0x20: a=r16(c,c->pc); c->pc+=2; push(c,(uint8_t)((c->pc-1)>>8)); push(c,(uint8_t)(c->pc-1)); c->pc=a; break;
    case 0x21: a=r16z(c,rd(c,c->pc++))+c->x;             c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x23: a=am_idx(c);                               rla(c,a);                          break;
    case 0x24: a=rd(c,c->pc++); v=rd(c,a); RF(c,FLAG_N|FLAG_V|FLAG_Z); if(v&0x80)SF(c,FLAG_N); if(v&0x40)SF(c,FLAG_V); if(!(v&c->a))SF(c,FLAG_Z); break;
    case 0x25: a=rd(c,c->pc++);                           c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x26: a=rd(c,c->pc++); rol_m(c,a); break;
    case 0x27: a=rd(c,c->pc++); rla(c,a); break;
    case 0x28: {uint8_t f=pop(c); c->flags=(f&0xEF)|FLAG_B;} break;
    case 0x29: c->a&=rd(c,c->pc++); NZ(c,c->a); break;
    case 0x2A: rol_a(c); break;
    case 0x2B: c->a&=rd(c,c->pc++); RF(c,FLAG_C); if(c->a&0x80)SF(c,FLAG_C); NZ(c,c->a); break;
    case 0x2C: a=r16(c,c->pc); c->pc+=2; v=rd(c,a); RF(c,FLAG_N|FLAG_V|FLAG_Z); if(v&0x80)SF(c,FLAG_N); if(v&0x40)SF(c,FLAG_V); if(!(v&c->a))SF(c,FLAG_Z); break;
    case 0x2D: a=r16(c,c->pc); c->pc+=2;                 c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x2E: a=r16(c,c->pc); c->pc+=2; rol_m(c,a); break;
    case 0x2F: a=r16(c,c->pc); c->pc+=2; rla(c,a); break;
    case 0x30: cy+=br(c,FL(c,FLAG_N)); break;
    case 0x31: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x33: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; rla(c,a); break;
    case 0x34: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0x35: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x36: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; rol_m(c,a); break;
    case 0x37: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; rla(c,a); break;
    case 0x38: SF(c,FLAG_C); break;
    case 0x39: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x3A: break;
    case 0x3B: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; rla(c,a); break;
    case 0x3C: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0x3D: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; c->a&=rd(c,a); NZ(c,c->a); break;
    case 0x3E: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; rol_m(c,a); break;
    case 0x3F: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; rla(c,a); break;
    case 0x40: c->flags=pop(c)|FLAG_B; c->pc=pop(c)|((uint16_t)pop(c)<<8); break;
    case 0x41: a=r16z(c,rd(c,c->pc++))+c->x;             c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x43: a=am_idx(c);                               sre(c,a);                          break;
    case 0x44: rd(c,c->pc++); break;
    case 0x45: a=rd(c,c->pc++);                           c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x46: a=rd(c,c->pc++); lsr_m(c,a); break;
    case 0x47: a=rd(c,c->pc++); sre(c,a); break;
    case 0x48: push(c,c->a); break;
    case 0x49: c->a^=rd(c,c->pc++); NZ(c,c->a); break;
    case 0x4A: RF(c,FLAG_C); if(c->a&1)SF(c,FLAG_C); c->a>>=1; NZ(c,c->a); break;
    case 0x4B: c->a&=rd(c,c->pc++); RF(c,FLAG_C); if(c->a&1)SF(c,FLAG_C); c->a>>=1; NZ(c,c->a); break;
    case 0x4C: c->pc=r16(c,c->pc); break;
    case 0x4D: a=r16(c,c->pc); c->pc+=2;                 c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x4E: a=r16(c,c->pc); c->pc+=2; lsr_m(c,a); break;
    case 0x4F: a=r16(c,c->pc); c->pc+=2; sre(c,a); break;
    case 0x50: cy+=br(c,!FL(c,FLAG_V)); break;
    case 0x51: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x53: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; sre(c,a); break;
    case 0x54: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0x55: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x56: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; lsr_m(c,a); break;
    case 0x57: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; sre(c,a); break;
    case 0x58: RF(c,FLAG_I); break;
    case 0x59: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x5A: break;
    case 0x5B: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; sre(c,a); break;
    case 0x5C: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0x5D: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; c->a^=rd(c,a); NZ(c,c->a); break;
    case 0x5E: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; lsr_m(c,a); break;
    case 0x5F: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; sre(c,a); break;
    case 0x60: c->pc=pop(c)|((uint16_t)pop(c)<<8); c->pc++; break;
    case 0x61: a=r16z(c,rd(c,c->pc++))+c->x;             adc(c,rd(c,a));                     break;
    case 0x63: a=am_idx(c);                               rra(c,a);                          break;
    case 0x64: rd(c,c->pc++); break;
    case 0x65: a=rd(c,c->pc++);                           adc(c,rd(c,a));                     break;
    case 0x66: a=rd(c,c->pc++); ror_m(c,a); break;
    case 0x67: a=rd(c,c->pc++); rra(c,a); break;
    case 0x68: c->a=pop(c); NZ(c,c->a); break;
    case 0x69: adc(c,rd(c,c->pc++)); break;
    case 0x6A: ror_a(c); break;
    case 0x6B: c->a&=rd(c,c->pc++); {uint8_t o=FL(c,FLAG_C)?0x80:0; if(c->a&1)SF(c,FLAG_C);else RF(c,FLAG_C); c->a=(c->a>>1)|o; RF(c,FLAG_V); SF(c,FLAG_C); if((c->a&0x40))SF(c,FLAG_C); else RF(c,FLAG_C); if((c->a&0x40)^((c->a&0x20)<<1))SF(c,FLAG_V); NZ(c,c->a);} break;
    case 0x6C: a=r16(c,c->pc); c->pc=rd(c,a)|((uint16_t)rd(c,(a&0xFF00)|((uint8_t)(a+1)))<<8); break;
    case 0x6D: a=r16(c,c->pc); c->pc+=2;                 adc(c,rd(c,a));                     break;
    case 0x6E: a=r16(c,c->pc); c->pc+=2; ror_m(c,a); break;
    case 0x6F: a=r16(c,c->pc); c->pc+=2; rra(c,a); break;
    case 0x70: cy+=br(c,FL(c,FLAG_V)); break;
    case 0x71: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; adc(c,rd(c,a)); break;
    case 0x73: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; rra(c,a); break;
    case 0x74: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0x75: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    adc(c,rd(c,a));                     break;
    case 0x76: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; ror_m(c,a); break;
    case 0x77: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; rra(c,a); break;
    case 0x78: SF(c,FLAG_I); break;
    case 0x79: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; adc(c,rd(c,a)); break;
    case 0x7A: break;
    case 0x7B: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; rra(c,a); break;
    case 0x7C: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0x7D: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; adc(c,rd(c,a)); break;
    case 0x7E: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; ror_m(c,a); break;
    case 0x7F: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; rra(c,a); break;
    case 0x80: rd(c,c->pc++); break;
    case 0x81: a=r16z(c,rd(c,c->pc++))+c->x;             wr(c,a,c->a);                       break;
    case 0x82: rd(c,c->pc++); break;
    case 0x83: a=am_idx(c);                               wr(c,a,c->a&c->x);                 break;
    case 0x84: a=rd(c,c->pc++);                           wr(c,a,c->y);                       break;
    case 0x85: a=rd(c,c->pc++);                           wr(c,a,c->a);                       break;
    case 0x86: a=rd(c,c->pc++);                           wr(c,a,c->x);                       break;
    case 0x87: a=rd(c,c->pc++);                           wr(c,a,c->a&c->x);                 break;
    case 0x88: c->y--; NZ(c,c->y); break;
    case 0x89: rd(c,c->pc++); break;
    case 0x8A: c->a=c->x; NZ(c,c->a); break;
    case 0x8B: {v=c->a|0xEE; v&=c->x; v&=rd(c,c->pc++); c->a=v; NZ(c,c->a);} break;
    case 0x8C: a=r16(c,c->pc); c->pc+=2;                  wr(c,a,c->y);                       break;
    case 0x8D: a=r16(c,c->pc); c->pc+=2;                  wr(c,a,c->a);                       break;
    case 0x8E: a=r16(c,c->pc); c->pc+=2;                  wr(c,a,c->x);                       break;
    case 0x8F: a=r16(c,c->pc); c->pc+=2;                  wr(c,a,c->a&c->x);                  break;
    case 0x90: cy+=br(c,!FL(c,FLAG_C)); break;
    case 0x91: a=am_idy(c)+c->y;                          wr(c,a,c->a);                       break;
    case 0x93: a=am_idy(c)+c->y;                          wr(c,a,c->a&c->x&((a>>8)+1));      break;
    case 0x94: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    wr(c,a,c->y);                       break;
    case 0x95: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    wr(c,a,c->a);                       break;
    case 0x96: a=((uint16_t)rd(c,c->pc++)+c->y)&0xFF;    wr(c,a,c->x);                       break;
    case 0x97: a=((uint16_t)rd(c,c->pc++)+c->y)&0xFF;    wr(c,a,c->a&c->x);                  break;
    case 0x98: c->a=c->y; NZ(c,c->a); break;
    case 0x99: a=r16(c,c->pc)+c->y; c->pc+=2;            wr(c,a,c->a);                       break;
    case 0x9A: c->sp=c->x; break;
    case 0x9B: a=r16(c,c->pc)+c->y; c->pc+=2; c->sp=c->a&c->x; wr(c,a,c->sp&((a>>8)+1)); break;
    case 0x9C: a=r16(c,c->pc)+c->x; c->pc+=2; wr(c,a,c->y&((a>>8)+1)); break;
    case 0x9D: a=r16(c,c->pc)+c->x; c->pc+=2;            wr(c,a,c->a);                       break;
    case 0x9E: a=r16(c,c->pc)+c->y; c->pc+=2; wr(c,a,c->x&((a>>8)+1)); break;
    case 0x9F: a=r16(c,c->pc)+c->y; c->pc+=2; wr(c,a,c->a&c->x&((a>>8)+1)); break;
    case 0xA0: c->y=rd(c,c->pc++); NZ(c,c->y); break;
    case 0xA1: a=r16z(c,rd(c,c->pc++))+c->x;             c->a=rd(c,a); NZ(c,c->a); break;
    case 0xA2: c->x=rd(c,c->pc++); NZ(c,c->x); break;
    case 0xA3: a=am_idx(c);                               c->a=c->x=rd(c,a); NZ(c,c->a);     break;
    case 0xA4: a=rd(c,c->pc++);                           c->y=rd(c,a); NZ(c,c->y); break;
    case 0xA5: a=rd(c,c->pc++);                           c->a=rd(c,a); NZ(c,c->a); break;
    case 0xA6: a=rd(c,c->pc++);                           c->x=rd(c,a); NZ(c,c->x); break;
    case 0xA7: a=rd(c,c->pc++);                           c->a=c->x=rd(c,a); NZ(c,c->a);     break;
    case 0xA8: c->y=c->a; NZ(c,c->y); break;
    case 0xA9: c->a=rd(c,c->pc++); NZ(c,c->a); break;
    case 0xAA: c->x=c->a; NZ(c,c->x); break;
    case 0xAB: v=rd(c,c->pc++); c->a=c->x=v; NZ(c,c->a); break;
    case 0xAC: a=r16(c,c->pc); c->pc+=2;                  c->y=rd(c,a); NZ(c,c->y); break;
    case 0xAD: a=r16(c,c->pc); c->pc+=2;                  c->a=rd(c,a); NZ(c,c->a); break;
    case 0xAE: a=r16(c,c->pc); c->pc+=2;                  c->x=rd(c,a); NZ(c,c->x); break;
    case 0xAF: a=r16(c,c->pc); c->pc+=2;                  c->a=c->x=rd(c,a); NZ(c,c->a);     break;
    case 0xB0: cy+=br(c,FL(c,FLAG_C)); break;
    case 0xB1: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; c->a=rd(c,a); NZ(c,c->a); break;
    case 0xB3: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; c->a=c->x=rd(c,a); NZ(c,c->a); break;
    case 0xB4: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    c->y=rd(c,a); NZ(c,c->y); break;
    case 0xB5: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    c->a=rd(c,a); NZ(c,c->a); break;
    case 0xB6: a=((uint16_t)rd(c,c->pc++)+c->y)&0xFF;    c->x=rd(c,a); NZ(c,c->x); break;
    case 0xB7: a=((uint16_t)rd(c,c->pc++)+c->y)&0xFF;    c->a=c->x=rd(c,a); NZ(c,c->a);     break;
    case 0xB8: RF(c,FLAG_V); break;
    case 0xB9: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->a=rd(c,a); NZ(c,c->a); break;
    case 0xBA: c->x=c->sp; NZ(c,c->x); break;
    case 0xBB: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->sp=c->sp&rd(c,a); c->x=c->sp; c->a=c->sp; break;
    case 0xBC: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; c->y=rd(c,a); NZ(c,c->y); break;
    case 0xBD: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; c->a=rd(c,a); NZ(c,c->a); break;
    case 0xBE: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->x=rd(c,a); NZ(c,c->x); break;
    case 0xBF: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; c->a=c->x=rd(c,a); NZ(c,c->a); break;
    case 0xC0: cmp(c,c->y,rd(c,c->pc++)); break;
    case 0xC1: a=r16z(c,rd(c,c->pc++))+c->x;             cmp(c,c->a,rd(c,a)); break;
    case 0xC2: rd(c,c->pc++); break;
    case 0xC3: a=am_idx(c);                               dcp(c,a);                          break;
    case 0xC4: a=rd(c,c->pc++);                           cmp(c,c->y,rd(c,a)); break;
    case 0xC5: a=rd(c,c->pc++);                           cmp(c,c->a,rd(c,a)); break;
    case 0xC6: a=rd(c,c->pc++); v=rd(c,a)-1; NZ(c,v); wr(c,a,v); break;
    case 0xC7: a=rd(c,c->pc++); dcp(c,a); break;
    case 0xC8: c->y++; NZ(c,c->y); break;
    case 0xC9: cmp(c,c->a,rd(c,c->pc++)); break;
    case 0xCA: c->x--; NZ(c,c->x); break;
    case 0xCB: {v=rd(c,c->pc++); uint16_t t=(uint16_t)(c->a&c->x)-v; RF(c,FLAG_C); if((c->a&c->x)>=v)SF(c,FLAG_C); c->x=(uint8_t)t; NZ(c,c->x);} break;
    case 0xCC: a=r16(c,c->pc); c->pc+=2;                  cmp(c,c->y,rd(c,a)); break;
    case 0xCD: a=r16(c,c->pc); c->pc+=2;                  cmp(c,c->a,rd(c,a)); break;
    case 0xCE: a=r16(c,c->pc); c->pc+=2; v=rd(c,a)-1; NZ(c,v); wr(c,a,v); break;
    case 0xCF: a=r16(c,c->pc); c->pc+=2; dcp(c,a); break;
    case 0xD0: cy+=br(c,!FL(c,FLAG_Z)); break;
    case 0xD1: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; cmp(c,c->a,rd(c,a)); break;
    case 0xD3: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; dcp(c,a); break;
    case 0xD4: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0xD5: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    cmp(c,c->a,rd(c,a)); break;
    case 0xD6: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; v=rd(c,a)-1; NZ(c,v); wr(c,a,v); break;
    case 0xD7: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; dcp(c,a); break;
    case 0xD8: RF(c,FLAG_D); break;
    case 0xD9: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; cmp(c,c->a,rd(c,a)); break;
    case 0xDA: break;
    case 0xDB: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; dcp(c,a); break;
    case 0xDC: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0xDD: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; cmp(c,c->a,rd(c,a)); break;
    case 0xDE: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; v=rd(c,a)-1; NZ(c,v); wr(c,a,v); break;
    case 0xDF: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; dcp(c,a); break;
    case 0xE0: cmp(c,c->x,rd(c,c->pc++)); break;
    case 0xE1: a=r16z(c,rd(c,c->pc++))+c->x;             sbc(c,rd(c,a)); break;
    case 0xE2: rd(c,c->pc++); break;
    case 0xE3: a=am_idx(c);                               isb(c,a);                          break;
    case 0xE4: a=rd(c,c->pc++);                           cmp(c,c->x,rd(c,a)); break;
    case 0xE5: a=rd(c,c->pc++);                           sbc(c,rd(c,a)); break;
    case 0xE6: a=rd(c,c->pc++); v=rd(c,a)+1; NZ(c,v); wr(c,a,v); break;
    case 0xE7: a=rd(c,c->pc++); isb(c,a); break;
    case 0xE8: c->x++; NZ(c,c->x); break;
    case 0xE9: sbc(c,rd(c,c->pc++)); break;
    case 0xEA: break;
    case 0xEB: sbc(c,rd(c,c->pc++)); break;
    case 0xEC: a=r16(c,c->pc); c->pc+=2;                  cmp(c,c->x,rd(c,a)); break;
    case 0xED: a=r16(c,c->pc); c->pc+=2;                  sbc(c,rd(c,a)); break;
    case 0xEE: a=r16(c,c->pc); c->pc+=2; v=rd(c,a)+1; NZ(c,v); wr(c,a,v); break;
    case 0xEF: a=r16(c,c->pc); c->pc+=2; isb(c,a); break;
    case 0xF0: cy+=br(c,FL(c,FLAG_Z)); break;
    case 0xF1: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; sbc(c,rd(c,a)); break;
    case 0xF3: a=am_idy(c)+c->y; cy+=((a^c->y)>>8)&1; isb(c,a); break;
    case 0xF4: rd(c,((uint16_t)rd(c,c->pc++)+c->x)&0xFF); break;
    case 0xF5: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF;    sbc(c,rd(c,a)); break;
    case 0xF6: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; v=rd(c,a)+1; NZ(c,v); wr(c,a,v); break;
    case 0xF7: a=((uint16_t)rd(c,c->pc++)+c->x)&0xFF; isb(c,a); break;
    case 0xF8: SF(c,FLAG_D); break;
    case 0xF9: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; sbc(c,rd(c,a)); break;
    case 0xFA: break;
    case 0xFB: a=r16(c,c->pc)+c->y; c->pc+=2; cy+=((a^c->y)>>8)&1; isb(c,a); break;
    case 0xFC: rd(c,r16(c,c->pc)+c->x); c->pc+=2; cy+=((a^c->x)>>8)&1; break;
    case 0xFD: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; sbc(c,rd(c,a)); break;
    case 0xFE: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; v=rd(c,a)+1; NZ(c,v); wr(c,a,v); break;
    case 0xFF: a=r16(c,c->pc)+c->x; c->pc+=2; cy+=((a^c->x)>>8)&1; isb(c,a); break;
    }

    c->cycles+=cy;
    return cy;
}