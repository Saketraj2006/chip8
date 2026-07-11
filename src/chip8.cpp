#include "chip8.hpp"
#include <cstdlib>
#include <iostream>
const uint8_t fontset[80]={
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(){
    // setting pc to start of memory
    pc=0x200u;

    // load the fontset into system memory
    for(int i=0;i<80;i++){
        memory[0x050u+i]=fontset[i];
    }
}

void Chip8::cycle(){
    //Fetch
    uint16_t opcode=(memory[pc]<<8u)|memory[pc+1];
    pc+=2;

    //Decode & execute
    uint8_t X=(opcode & 0x0F00u)>>8u;
    uint8_t Y=(opcode & 0x00F0u)>>4u;
    uint8_t N=(opcode & 0x000Fu);
    uint8_t NN=(opcode & 0x00FFu);
    uint16_t NNN=(opcode & 0x0FFFu);

    switch(opcode & 0xF000u)
    {
    case 0x0000:
        switch (NN)
        {
        case 0xE0u:
            display={};
            break;
        
        case 0xEEu:
            if(sp==0) break;
            sp--;
            pc=stack[sp];
            break;
        
        default:
            // opcode 0NNN: SYS addr
            std::cout<<"Hardware warning: Ignored RCA 1802 SYS call at address: "<<std::hex<<NNN<<"\n";
            break;
        }
        break;
        
    case 0x1000:
        pc=NNN;
        break;

    case 0x2000:
        if(sp>=16) break;
        stack[sp]=pc;
        sp++;
        pc=NNN;
        break;

    case 0x3000:
        if(V[X]==NN) pc+=2;
        break;
        
    case 0x4000:
        if(V[X]!=NN) pc+=2;
        break;

    case 0x5000:
        if(V[X]==V[Y]) pc+=2;
        break;
    
    case 0x6000:
        V[X]=NN;
        break;

    case 0x7000:
        V[X]+=NN;
        break;
    
    case 0x8000:
    {

        switch(opcode&0x000Fu)
        {
        case 0x0000:
            V[X]=V[Y];
            break;
        
        case 0x0001:
            V[X]|=V[Y];
            break;

        case 0x0002:
            V[X]&=V[Y];
            break;

        case 0x0003:
            V[X]^=V[Y];
            break;

        case 0x0004:
        {
            uint16_t sum=V[X]+V[Y];
            if(sum>255u) V[0xF]=1;
            else V[0xF]=0;
            V[X]=sum & 0xFFu;
            break;
        }
        case 0x0005:
            if(V[X]>=V[Y]) V[0xF]=1;
            else V[0xF]=0;
            V[X]-=V[Y];
            break;
        
        case 0x0006:
        {
            uint8_t flag=V[X]&0x1u;
            V[X]>>=1;
            V[0xF]=flag;
            break;
        }

        case 0x0007:
            if(V[Y]>=V[X]) V[0xF]=1;
            else V[0xF]=0;
            V[X]=V[Y]-V[X];
            break;
        
        case 0x0000E:
        {
            uint8_t flag=(V[X]&0x80u)>>7u;
            V[X]<<=1;
            V[0xF]=flag;
            break;
        }

        default:
            break;
        }
        break;
    }

    case 0x9000:
        if(V[X]!=V[Y]) pc+=2;
        break;

    case 0xA000:
        I=NNN;
        break;

    case 0xB000:
        pc=V[0]+NNN;
        break;

    case 0xC000:
        V[X]=(rand()%256)&NN;
        break;

    case 0xD000:
    {
        uint8_t xPos=V[X]%64;
        uint8_t yPos=V[Y]%32;
        V[0xF]=0;

        for(unsigned int row=0;row<N;row++){
            if(I+row>=4096) break;
            uint8_t spriteByte=memory[I+row];

            for(unsigned int col=0;col<8;col++){
                uint8_t spritePixel=spriteByte & (0x80u>>col);
                uint32_t screenX=xPos+col;
                uint32_t screenY=yPos+row;
                
                if(screenX>=64) break;
                if(screenY>=32) break;

                if(spritePixel){
                    uint32_t screenPixelIndex=(screenY)*64+(screenX);

                    if(display[screenPixelIndex]==1) V[0xF]=1;
                    display[screenPixelIndex]^=1;
                }
            }
        }
       break;
    }

    case 0xE000:
    {
        switch(NN)
        {
        case 0x9E:
            if(V[X]<=0xF){
                if(keypad[V[X]]) pc+=2;
            }
            break;

        case 0xA1:
            if(V[X]>0xF || !keypad[V[X]]) pc+=2;
            break;
        
        default:
            break;
        }
        break;
    }

    case 0xF000:
    {
        switch(NN)
        {
        case 0x07:
            V[X]=delayTimer;
            break;

        case 0x0A:{
            bool keyPress=false;
            for(int i=0;i<16;i++){
                if(keypad[i]){
                    V[X]=i;
                    keyPress=true;
                    break;
                }
            }
            if(!keyPress) pc-=2;
            break;
        }

        case 0x15:
            delayTimer=V[X];
            break;

        case 0x18:
            soundTimer=V[X];
            break;

        case 0x1E:
            I+=V[X];
            break;

        case 0x29:
            I=0x050+(V[X]*5);
            break;

        case 0x33:{
            uint8_t value=V[X];
            memory[I]=value/100;
            memory[I+1]=(value/10)%10;
            memory[I+2]=value%10;
            break;
        }

        case 0x55:
        {
            for(int i=0;i<=X;i++){
                if(I+i>=4096) break;
                memory[I+i]=V[i];
            }
            break;
        }

        case 0x65:{
            for(int i=0;i<=X;i++){
                if(I+i>=4096) break;
                V[i]=memory[I+i];
            }
            break;
        }

        default:
            break;
        }
        break;
    }

    default:
        std::cerr<<"Unknown opcode: "<<std::hex<<opcode<<std::endl;
        break;
    }
}

bool Chip8::loadROM(const char *filename)
{
    std::ifstream file(filename,std::ios::binary | std::ios::ate);
    if(file.is_open()){
        std::streampos size=file.tellg();
        const std::streampos MAX_AVL=4096-0x200;
        if(size>MAX_AVL) return false;
        
        file.seekg(0,std::ios::beg);
        file.read(reinterpret_cast<char *>(&memory[0x200]),size);
        file.close();

        return true;
    }
    std::cerr<<"Failed to open ROM\n";
    return false;
}