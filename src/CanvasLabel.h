// このヘッダーファイルを重複して #include されたときに、定義が重複エラーになるのを避けるようにします。
#pragma once

#include <M5Stack.h>
#include <M5GFX.h>

class CanvasLabel{

  private:
    int _x;
    int _y;
    int _width;
    int _height;
    String _caption;
    M5GFX& _lcd;
    M5Canvas _canvas;
    int _textLenght;
    int _textWidth;
    int _textHeight;

    void SetupCanvas();
    void ClearText();


  // 型の設定
  public:
    CanvasLabel(M5GFX& lcd, int x, int y, int width, int height);
    CanvasLabel(M5GFX& lcd, String caption, int x, int y, int width, int height);
    void UpdateLabel(String caption);
    void UpdateLabelAsRight(String caption);
    void AddLabelText(String caption);
    void AddLabelText(String caption,bool isReturn);
    int GetTextLength();
    int32_t GetTextWidth();
    int32_t GetTextHeight();
    String GetCaption();
};
