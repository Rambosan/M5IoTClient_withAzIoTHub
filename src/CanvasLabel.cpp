#include "CanvasLabel.h"
#include <M5Stack.h>
#include <M5GFX.h>

// コンストラクタ
CanvasLabel::CanvasLabel(M5GFX& lcd, int x, int y, int width,int height):
  _x(x), _y(y), _width(width), _height(height),
  _caption(""),
  _lcd(lcd),
  _canvas(new M5Canvas(&lcd))
{ 
    // canvasを初期化
    SetupCanvas();
}

CanvasLabel::CanvasLabel(M5GFX& lcd, String caption, int x, int y, int width,int height):
  _x(x), _y(y), _width(width), _height(height),
  _caption(caption),
  _lcd(lcd),
  _canvas(new M5Canvas(&lcd))
{
    // canvasを初期化
    SetupCanvas();
    UpdateLabel(_caption);
}

// 現在表示されているキャプションを取得します。
String CanvasLabel::GetCaption(){
  return _caption;
}

// ラベルに表示するメッセージを更新します。
void CanvasLabel::UpdateLabel(String caption){
  //テキストを消去
  ClearText();

  // 文字列描画
  _canvas.println(caption);
  // 指定座標にcanvas描画
  _canvas.pushSprite(&_lcd, _x, _y);

  // 更新後の文字のプロパティをセット
  _caption = caption;  
}

// ラベルに表示される文字を右寄せで更新します。
void CanvasLabel::UpdateLabelAsRight(String caption){
  //テキストを消去
  ClearText();

  // 文字列描画
  _canvas.drawRightString(caption,0,0);
  // 指定座標にcanvas描画
  _canvas.pushSprite(&_lcd, _x, _y);

  // 更新後の文字のプロパティをセット
  _caption = caption;  
}

// 既存のラベルにメッセージを追加します。
void CanvasLabel::AddLabelText(String caption){
  CanvasLabel::AddLabelText(caption,false);
}

// 既存のラベルにメッセージを追加します。
void CanvasLabel::AddLabelText(String caption,bool isReturn = false){

  // 文字列描画
  if(isReturn == true){
    _canvas.println(caption);
  }else{
    _canvas.print(caption);
  }

  // canvasの指定座標に描画
  _canvas.pushSprite(&_lcd,_x, _y);

  // 更新後の文字のプロパティをセット
  _caption = caption;
}

// テキスト長さを取得
int CanvasLabel::GetTextLength(){
  return _caption.length();
}

// テキストの幅を取得する
int32_t CanvasLabel::GetTextWidth(){
  return _canvas.textWidth(_caption);
}

// フォントの高さを取得
int32_t CanvasLabel::GetTextHeight(){
  return _textHeight = _canvas.fontHeight();
}



void CanvasLabel::SetupCanvas(){
    _canvas.setColorDepth(2);
    _canvas.createSprite(_width, _height);
    _canvas.fillRect(0, 0, _width, _width,TFT_BLACK);
    _canvas.setTextColor(WHITE);
    _canvas.setFont(&fonts::lgfxJapanGothic_16);
}

void CanvasLabel::ClearText(){
    _canvas.fillRect(0, 0, _width, _width,TFT_BLACK);
    _canvas.setCursor(0,0);
}