#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "clay_renderer_raylib.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


Clay_Color mainBackgroundColor={0xE5,0xE5,0xE5,0xFF};
Clay_Color mainForegroundColor={0x0F,0x0F,0x0F,0xFF};

int targetFPS=60;


void HandleClayErrors(Clay_ErrorData errorData) {
    printf("%s", errorData.errorText.chars);
}

int main(){

	Clay_Raylib_Initialize(640,480,"Clay Unicode",FLAG_WINDOW_RESIZABLE|FLAG_WINDOW_HIGHDPI|FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT); // Extra parameters to this function are new since the video was published

	uint64_t clayRequiredMemory=Clay_MinMemorySize();
	Clay_Arena clayMemory=Clay_CreateArenaWithCapacityAndMemory(clayRequiredMemory,malloc(clayRequiredMemory));

	Clay_Initialize(
		clayMemory,
		(Clay_Dimensions){.width=GetScreenWidth(),.height=GetScreenHeight()},
		(Clay_ErrorHandler){HandleClayErrors,NULL}
	);

	//this is for the example. you should have a splash screen up before loading fonts.
	Font fonts[2];
	fonts[0] = LoadFontEx("NotoSansJP-Regular.otf",48,0,0xA000);
	SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_BILINEAR);
	fonts[1] = LoadFontEx("NotoSansJP-Bold.otf",48,0,0xA000);
	SetTextureFilter(fonts[1].texture, TEXTURE_FILTER_BILINEAR);
	Clay_SetMeasureTextFunction(Raylib_MeasureText, fonts);
	
	//loading some text into memory
	Clay_Unicode demotext=Clay_MakeUnicode(
		"This is Clay Unicode, a minor fork for Clay made necessary for my project.\n\nIt displays and highlights in unicode: 『カラマーゾフの​兄弟』​は、​ロシアの​文学者​フョードル​・​ドストエフスキーの​最後の​長編小説。\n\nIt's not perfect, but it sure is useful.",
		mainForegroundColor,0,48,48,0.0f,true,CLAY_ALIGN_X_CENTER,CLAY_ALIGN_Y_CENTER,1,1,115,12,(Clay_Color){0xFF,0x0F,0x0F,0xFF}
	);
	
	


	while(!WindowShouldClose()){
		Clay_SetLayoutDimensions((Clay_Dimensions){.width=GetScreenWidth(),.height=GetScreenHeight()});

		Clay_BeginLayout();
		
		//boilerplate
		CLAY({//body, background, whatever
			.id=CLAY_ID("body"),
			.backgroundColor=mainBackgroundColor,
			.layout={
				.sizing={.width=CLAY_SIZING_GROW(0),.height=CLAY_SIZING_GROW(0)},
				.layoutDirection=CLAY_TOP_TO_BOTTOM
			}
		}){
			CLAY({//container, aligns stuff to center
				.layout={
					.sizing={.width=CLAY_SIZING_GROW(0),.height=CLAY_SIZING_GROW(0)},
					.padding={15,15,15,15},
					.childAlignment = {CLAY_ALIGN_X_CENTER,CLAY_ALIGN_Y_CENTER}
				}
			}){
				CLAY({//box to hold text
					.layout={
						.sizing={.width=CLAY_SIZING_GROW(0),.height=CLAY_SIZING_GROW(0)},
						.padding={15,15,15,15}
					},
					.cornerRadius={5,5,5,5},
					.border={.width=CLAY_BORDER_OUTSIDE(1),.color={0x66,0x66,0x66,0xFF}}
				}){
					CLAY({
						
						.layout={
							.sizing={.width=CLAY_SIZING_GROW(0),.height=CLAY_SIZING_GROW(0)},
						},
						.backgroundColor=(Clay_Color){0xFF,0xFF,0x00,0x4F},
						.userData=&demotext
					}){}
				}
			}
		}
		

		Clay_RenderCommandArray renderCommands=Clay_EndLayout();

		BeginDrawing();
		ClearBackground(BLACK);
		Clay_Raylib_Render(renderCommands,fonts);
		EndDrawing();
	}
	Clay_Raylib_Close();
	return 0;
}