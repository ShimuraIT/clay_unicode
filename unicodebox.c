#include "raylib.h"
//#include "linkedlists.h"//this is my linked list library, some of whose functions i imported here. it's not good to ship yet.
//using linked lists for strings is crazy inefficient but you wanna know something?
//...
//i have no excuse. the thing i made this for is taking months to make. i might fix this up later.
//once again: i apologize for lack of comments. this code was heavily based on the work of Vlad Adrian at https://www.raylib.com/examples/text/loader.html?name=text_unicode_emojis
//this is extremely inefficient to run every frame because it parses the text *every time.* saving to a unicodebox struct and printing from that would be much better.


#include <stdio.h>
#include <string.h>

///linked lists
//i dont want this popping up for you in an IDE so i renamed everything  "_CUxyz" (clay unicode) and got rid of the struct typedef.
//if i ever put up my linked list code, you can include it and remove everything below to line ~150.
//fix the renaming in the unicodebox code with find+replace "struct _CUllnode" -> "llnode" and "_CU" and "_CUnode" -> "node"

//node struct
struct _CUllnode{
	void* var;			//variable
	uint16_t nid;		//numeric id	-	for things with lots of numbers
	char* sid;			//string id		-	for things with aliases
	bool ref;			//reference flag-	false=free memory when done
	struct _CUllnode* prev;
	struct _CUllnode* next;
};

//internal
struct _CUllnode* _CUnode_new(uint16_t nid, char* sid, void* var, struct _CUllnode* prev, struct _CUllnode* next, bool reference);
void _CUnode_delete(struct _CUllnode* n);

//external
struct _CUllnode* _CUnode_append(uint16_t nid, char* sid, void* var, struct _CUllnode* list);
struct _CUllnode* _CUnode_list_start(struct _CUllnode* node);
struct _CUllnode* _CUnode_list_end(struct _CUllnode* node);
uint32_t _CUnode_list_length(struct _CUllnode* list);
struct _CUllnode* _CUnode_list_seek(struct _CUllnode* list, int16_t count);
void _CUnode_list_destroy(struct _CUllnode* n);
struct _CUllnode* _CUnode_eat(struct _CUllnode* n);

struct _CUllnode* _CUnode_new(uint16_t nid, char* sid, void* var, struct _CUllnode* prev, struct _CUllnode* next, bool ref){
	struct _CUllnode* n=malloc(sizeof(struct _CUllnode));
	n->var=var;
	n->nid=nid;
	n->sid=sid;
	n->ref=ref;
	n->prev=prev;
	n->next=next;
	return n;
}

void _CUnode_delete(struct _CUllnode* n){
	if(!n->ref){
		if(n->var!=NULL)free(n->var);
		if(n->sid!=NULL)free(n->sid);
	}
	free(n);
}


struct _CUllnode* _CUnode_new_list(uint16_t nid, char* sid, void* var, bool ref){
	return _CUnode_new(nid,sid,var,NULL,NULL,ref);
}

struct _CUllnode* _CUnode_append(uint16_t nid, char* sid, void* var, struct _CUllnode* list){
	if(list!=NULL){
		if(list->next!=NULL)list=_CUnode_list_end(list);
		list->next=_CUnode_new(nid,sid,var,list,NULL,list->ref);//clean fuckin code dawg, divine intellect type
		return list->next;
	}else{
		return _CUnode_new_list(nid,sid,var,false);//append... to nothing!
	}
}

struct _CUllnode* _CUnode_list_seek(struct _CUllnode* list, int16_t count){
	if(count<0){
		while(count!=0){
			if(list->prev!=NULL){
				list=list->prev;
				++count;
			}else break;
		}
	}else{
		while(count!=0){
			if(list->next!=NULL){
				list=list->next;
				--count;
			}else break;
		}
	}
	return list;
}

struct _CUllnode* _CUnode_list_start(struct _CUllnode* n){
	while(n->prev!=NULL)n=n->prev;
	return n;
}

struct _CUllnode* _CUnode_list_end(struct _CUllnode* n){
	while(n->next!=NULL)n=n->next;
	return n;
}

uint32_t _CUnode_list_length(struct _CUllnode* list){
	uint32_t r=1;
	struct _CUllnode* start=list;
	while(start->prev!=NULL){
		start=start->prev;
		++r;
	}
	while(list->next!=NULL){
		list=list->next;
		++r;
	}
	return r;
}

void _CUnode_list_destroy(struct _CUllnode* n){
	struct _CUllnode* r=n->next;
	struct _CUllnode* s;
	while(r!=NULL){
		s=r;
		r=r->next;
		_CUnode_delete(s);
	}
	r=n->prev;
	while(r!=NULL){
		s=r;
		r=r->prev;
		_CUnode_delete(s);
	}
	_CUnode_delete(n);
}

struct _CUllnode* _CUnode_eat(struct _CUllnode* n){
	struct _CUllnode* r=NULL;
	if(n!=NULL){
		if(n==n->next){//last one on a ring list.
			_CUnode_delete(n);
		}else{
			if(n->prev!=NULL){
				n->prev->next=n->next;
				r=n->prev;
			}
			if(n->next!=NULL){
				n->next->prev=n->prev;
				r=n->next;
			}
		}
	}
	_CUnode_delete(n);
	return r;
}


///internal structs
struct codepointdrawglyph{
	int codepoint;
	float xoffset;
	float glyphWidth;
	bool highlight;
	bool drawGlyph;
	bool isSpace;
};

struct codepointline{
	float width;
	float xoffset;//uniform lineheight
	struct _CUllnode* text;
};

static void DrawTextBoxedSelectable(Font font, Font selectFont, const char *text, Rectangle rec, float fontSize, float lineheight, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint, uint8_t halign, uint8_t valign);    // Draw text using font inside rectangle limits with support for text selection

static void DrawTextBoxedSelectable(Font font,Font selectFont,const char *text,Rectangle rec,float fontSize,float lineheightinput,float spacing,bool wordWrap,Color tint,int selectStart,int selectLength,Color selectTint,Color selectBackTint,uint8_t halign,uint8_t valign){
	const int dontdrawchars[]={'\n','\t','\r',0x200B,0};//formerly spaces
	const size_t ddarraylen=sizeof(dontdrawchars)/sizeof(int);//formerly spacearraylen

	const int spaces[]={' ',0x3000};//TODO: add spaces here for *every language*... see why it's todo? See also: http://jkorpela.fi/chars/spaces.html
	const size_t spacearraylen=sizeof(spaces)/sizeof(int);

	int length=TextLength(text)+1; // Total length in bytes of the text,scanned by codepoints in loop

	float textOffsetY=0.0f;	   // Offset between lines (on line break '\n')
	float textOffsetX=0.0f;	   // Offset X to next character to draw

	float scaleFactor=fontSize/(float)font.baseSize;	 // Character rectangle scaling factor
	float lineheight=(float)font.baseSize*scaleFactor;
	float linespacing=(font.baseSize+(font.baseSize*((lineheightinput/fontSize)-1)))*scaleFactor;// Tweak later. maybe add lineheight?

	//DrawRectangleRec(rec,selectBackTint);//debug

	uint16_t lineno=0;

	struct _CUllnode* drawabletext=NULL;
	struct _CUllnode* interline=NULL;

	int selectEnd=selectStart+selectLength;
	// Set up text to draw
	for(int i=0,k=0;i<length;i++,k++){
		interline=_CUnode_append(0,NULL,malloc(sizeof(struct codepointdrawglyph)),interline);
		struct codepointdrawglyph* character=(struct codepointdrawglyph*)interline->var;
		if((selectStart>=0)&&(k>=selectStart)&&(k<selectEnd)){
			character->highlight=true;
		}else{
			character->highlight=false;
		}
		character->xoffset=textOffsetX;
		character->isSpace=false;


		// Get next codepoint from byte string and glyph index in font
		int codepointByteCount=0;
		character->codepoint=GetCodepoint(&text[i],&codepointByteCount);
		int index=GetGlyphIndex(font,character->codepoint);

		// NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
		// but we need to draw all of the bad bytes using the '?' symbol moving one byte
		if(character->codepoint==0x3f)codepointByteCount=1;
		i+=(codepointByteCount - 1);

		
		character->drawGlyph=true;
		for(uint16_t k=0;k<ddarraylen;k++){//dont draw weird characters
			if(character->codepoint==dontdrawchars[k]){
				character->drawGlyph=false;
				character->glyphWidth=0;
				break;
			}
		}
		if(character->drawGlyph){//not weird character
			for(uint16_t k=0;k<spacearraylen;k++){//dont draw spaces
				if(character->codepoint==spaces[k]){//if it's a space dont bother drawing
					character->drawGlyph=false;
					character->isSpace=true;
					break;
				}
			}
			//set up glyph width, even for spaces
			character->glyphWidth=(font.glyphs[index].advanceX==0)? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;
			if(i+1<length)character->glyphWidth=character->glyphWidth+spacing;
		}

		//advanced line breaking because some languages need zwsp to break sense (like "make sense" get it haha)

		bool longbreak=((textOffsetX+character->glyphWidth)>rec.width);
		bool mandatorybreak=false;
		bool repairbreak=false;
		struct _CUllnode* nonline=interline;

		if(!((character->codepoint=='\n')||(character->codepoint==0))){
			if(longbreak&&wordWrap){
				if((!character->isSpace)&&(character->codepoint!=0)){
					bool totalbreak=false;
					if(interline->next=NULL)totalbreak=true;
					if(!totalbreak){
						while(interline!=NULL){
							character=(struct codepointdrawglyph*)interline->var;
							if(character->isSpace){
								character->isSpace=false;
								character->codepoint='\n';
								character->drawGlyph=false;
								character->glyphWidth=0;
								interline->next->prev=NULL;
								interline->next=NULL;
								totalbreak=true;
								repairbreak=true;
								break;
							}
							interline=interline->prev;
						}
					}
					if(!totalbreak){
						interline=nonline;
						while(interline!=NULL){
							character=(struct codepointdrawglyph*)interline->var;
							if(character->codepoint==0x200B){
								character->codepoint='\n';
								character->drawGlyph=false;
								character->glyphWidth=0;
								interline->next->prev=NULL;
								interline->next=NULL;
								totalbreak=true;
								repairbreak=true;
								break;
							}
							interline=interline->prev;
						}
					}
					if(!totalbreak){
						interline=nonline;
						character=(struct codepointdrawglyph*)interline->var;
						mandatorybreak=true;
					}
				}else{
					mandatorybreak=true;
				}
			}
		}
		

		float remx=0;
		if((character->codepoint=='\n')||(character->codepoint==0)||mandatorybreak){
			textOffsetX=0;
			drawabletext=_CUnode_append(lineno++,NULL,malloc(sizeof(struct codepointline)),drawabletext);
			struct codepointline* cpln=(struct codepointline*)drawabletext->var;
			cpln->xoffset=0;
			cpln->width=0;
			cpln->text=_CUnode_list_start(interline);
			interline=cpln->text;
			interline->nid=(uint16_t)_CUnode_list_length(interline);
			while(interline!=NULL){
				cpln->width+=((struct codepointdrawglyph*)interline->var)->glyphWidth;
				interline=interline->next;
			}
			remx=cpln->width;
		}else{
			textOffsetX+=character->glyphWidth;
		}

		if(repairbreak){
			interline=nonline;
			nonline=_CUnode_list_start(nonline);

			//textOffsetX=character->xoffset+character->glyphWidth-remx;//why is glyphwidth not width of glyph? please.
			/*do{
				character=(struct codepointdrawglyph*)nonline->var;
				character->xoffset-=remx;
				nonline=nonline->prev;
			}while(nonline!=NULL);*/
			
			do{
				character=(struct codepointdrawglyph*)nonline->var;
				character->xoffset=textOffsetX;
				textOffsetX+=character->glyphWidth;
				nonline=nonline->next;
			}while(nonline!=NULL);
		}
	}

	
	

	//set up alignment
	struct _CUllnode* endtext=drawabletext;
	drawabletext=_CUnode_list_start(drawabletext);
	struct _CUllnode* starttext=drawabletext;
	if(valign!=CLAY_ALIGN_Y_TOP){
		textOffsetY=rec.height-((endtext->nid*linespacing)+lineheight);
		if(valign==CLAY_ALIGN_Y_CENTER)textOffsetY/=2;
		textOffsetY=floor(textOffsetY);
	}
	if(halign!=CLAY_ALIGN_X_LEFT){
		for(uint16_t i=0;i<lineno;i++){
			struct codepointline *cpln=(struct codepointline*)starttext->var;
			cpln->xoffset=rec.width-cpln->width;
			if(halign==CLAY_ALIGN_X_CENTER)cpln->xoffset/=2;
			cpln->xoffset=cpln->xoffset;
			starttext=_CUnode_list_seek(starttext,1);
		}
	}
	Vector2 boxsz={
		.y=rec.y+rec.height-(lineheight*0.8),//the 0.8 is a weird hacky thing but it works pretty well.
		.x=rec.x+rec.width
	};
	for(uint16_t i=0;i<lineno;i++){
		struct codepointline *cpln=(struct codepointline*)drawabletext->var;
		float liney=floor((linespacing*drawabletext->nid)+textOffsetY+rec.y);
		
		interline=cpln->text;
		uint16_t charcnt=interline->nid;
		//DrawRectangleRec((Rectangle){rec.x+cpln->xoffset, liney, cpln->width, lineheight },selectBackTint);//debug
		if(!((liney+lineheight<0)||(liney>boxsz.y))){//if off screen don't render
			for(uint16_t j=0;j<charcnt;j++){
				struct codepointdrawglyph *character=(struct codepointdrawglyph*)interline->var;

				Font* dfont=&font;
				Color* dcolor=&tint;

				float charx=floor(rec.x+character->xoffset+cpln->xoffset);
				if(charx>boxsz.x)break;
				
				// Draw selection background
				if(character->highlight){
					DrawRectangleRec((Rectangle){charx, liney, character->glyphWidth, lineheight },selectBackTint);

					// swap colors
					dfont=&selectFont;
					dcolor=&selectTint;
				}
				// Draw current character glyph
				if(character->drawGlyph){
					DrawTextCodepoint(*dfont,character->codepoint,(Vector2){charx,liney },fontSize,*dcolor);
					
				}
				interline=_CUnode_list_seek(interline,1);
			}
		}
		_CUnode_list_destroy(interline);
		drawabletext=_CUnode_eat(drawabletext);
	}
}