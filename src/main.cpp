#include "chip8.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <fstream>
#include <ctime>
const int VIDEO_SCALE=15;

int main(int argc,char* argv[]){
    if(argc<2){
        std::cerr<<"Usage: "<<argv[0]<<" <ROM> [speed]\n";
        return -1;
    }

    int cyclesPerFrame=(argc==3)?std::stoi(argv[2]):10;
    bool isPaused=false;
    Chip8 chip8;
    if(!chip8.loadROM(argv[1])){
        std::cerr<<"Failed to load ROM\n";
        return -1;
    }

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);

    SDL_AudioSpec audioSpec={SDL_AUDIO_S16,1,44100};
    SDL_AudioStream* audioStream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audioSpec,nullptr,nullptr);
    SDL_ResumeAudioStreamDevice(audioStream);
    int audioPhase=0;

    SDL_Window* window=SDL_CreateWindow("CHIP-8 Emulator", 64*VIDEO_SCALE , 32*VIDEO_SCALE, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer=SDL_CreateRenderer(window, nullptr);
    SDL_Texture* texture=SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64,32);
    SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST);
    
    uint32_t pixels[64*32];
    uint8_t brightness[2048]={0};

    SDL_Event e;
    int currentTheme=0;

    srand(time(NULL));
    bool isRunning=true;
    while(isRunning){
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_EVENT_QUIT) isRunning=false;
            else if(e.type==SDL_EVENT_KEY_DOWN){
                switch(e.key.key){
                    case SDLK_SPACE: isPaused=!isPaused; break;

                    case SDLK_F5:{
                        std::ofstream out("save.state",std::ios::binary);
                        out.write(reinterpret_cast<char*>(&chip8),sizeof(Chip8));
                        std::cout<<"State Saved!\n";
                        break;
                    }

                    case SDLK_F7:{
                        std::ifstream in("save.state",std::ios::binary);
                        if(in.is_open()){
                            in.read(reinterpret_cast<char*>(&chip8),sizeof(Chip8));
                            for(int i=0;i<2048;i++) brightness[i]=0;
                            std::cout<<"State Loaded!\n";
                        }
                        else std::cout<<"No save state found!\n";
                        break;
                    }

                    case SDLK_T: currentTheme=(currentTheme+1)%3; break;

                    case SDLK_F1:{
                        chip8=Chip8();
                        chip8.loadROM(argv[1]);
                        for(int i=0;i<2048;i++) brightness[i]=0;
                        std::cout<<"System Reset!\n";
                        break;
                    }

                    case SDLK_N:{
                        if(isPaused) chip8.cycle();
                        break;
                    }
                    case SDLK_X: chip8.keypad[0]=1; break;
                    case SDLK_1: chip8.keypad[1]=1; break;
                    case SDLK_2: chip8.keypad[2]=1; break;
                    case SDLK_3: chip8.keypad[3]=1; break;
                    case SDLK_Q: chip8.keypad[4]=1; break;
                    case SDLK_W: chip8.keypad[5]=1; break;
                    case SDLK_E: chip8.keypad[6]=1; break;
                    case SDLK_A: chip8.keypad[7]=1; break;
                    case SDLK_S: chip8.keypad[8]=1; break;
                    case SDLK_D: chip8.keypad[9]=1; break;
                    case SDLK_Z: chip8.keypad[0xA]=1; break;
                    case SDLK_C: chip8.keypad[0xB]=1; break;
                    case SDLK_4: chip8.keypad[0xC]=1; break;
                    case SDLK_R: chip8.keypad[0xD]=1; break;
                    case SDLK_F: chip8.keypad[0xE]=1; break;
                    case SDLK_V: chip8.keypad[0xF]=1; break;
                }
            }
            else if(e.type==SDL_EVENT_KEY_UP){
                switch(e.key.key){
                    case SDLK_X: chip8.keypad[0]=0; break;
                    case SDLK_1: chip8.keypad[1]=0; break;
                    case SDLK_2: chip8.keypad[2]=0; break;
                    case SDLK_3: chip8.keypad[3]=0; break;
                    case SDLK_Q: chip8.keypad[4]=0; break;
                    case SDLK_W: chip8.keypad[5]=0; break;
                    case SDLK_E: chip8.keypad[6]=0; break;
                    case SDLK_A: chip8.keypad[7]=0; break;
                    case SDLK_S: chip8.keypad[8]=0; break;
                    case SDLK_D: chip8.keypad[9]=0; break;
                    case SDLK_Z: chip8.keypad[0xA]=0; break;
                    case SDLK_C: chip8.keypad[0xB]=0; break;
                    case SDLK_4: chip8.keypad[0xC]=0; break;
                    case SDLK_R: chip8.keypad[0xD]=0; break;
                    case SDLK_F: chip8.keypad[0xE]=0; break;
                    case SDLK_V: chip8.keypad[0xF]=0; break;
                }
            }
        }
        if(!isPaused){
            for(int i=0;i<cyclesPerFrame;i++) chip8.cycle();

            if(chip8.delayTimer>0) chip8.delayTimer--;
            if(chip8.soundTimer>0){
                // 1 frame audio-> 44100hz/60 fps =735 samples
                int16_t samples[735];
                for(int j=0;j<735;j++){
                    samples[j]=((audioPhase++/20)%2==0)?3000:-3000;
                }
                SDL_PutAudioStreamData(audioStream,samples,sizeof(samples));
                chip8.soundTimer--;
            }
            else audioPhase=0;
        }
        else SDL_ClearAudioStream(audioStream);

        for(int i=0;i<64*32;i++){
            if(chip8.display[i]==1) brightness[i]=255;
            else{
                if(brightness[i]>80) brightness[i]-=80;
                else brightness[i]=0;
            }

            uint8_t b=brightness[i];
            // White // Amber CRT // Hacker Green
            if(currentTheme==0) pixels[i]=(0xFF<<24)|(b<<16)|(b<<8)|b;
            else if(currentTheme==1) pixels[i]=(0xFF<<24)|(b<<16)|(static_cast<uint8_t>(b*0.6)<<8)|0;
            else pixels[i]=(0xFF<<24)|(0<<16)|(b<<8)|0;
        }

        SDL_UpdateTexture(texture, nullptr, pixels, 64*sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyAudioStream(audioStream);
    SDL_Quit();
    return 0;
}