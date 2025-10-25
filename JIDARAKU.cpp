/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * JIDARAKU.cpp
 * 
 * 自作配列「じだらく」
 *
 * Copyright 2025 TK Lab. All rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <imm.h>
#include <stdio.h>

#pragma comment( lib, "User32.lib" )
#pragma comment( lib, "Imm32.lib" )


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK HookProc(int nCode, WPARAM wp, LPARAM lp);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wp, LPARAM lp);
void HookStart();
void HookEnd();

char SendKey( char keyCode );
char SendKey( char modifierCode, char keyCode );

char SendKeyDown( char keyCode );
char SendKeyUp( char keyCode );

HINSTANCE hInst;
HHOOK hHook;
HHOOK hMouseHook;

HWND hWnd;

int lastKeyCode;
bool isLastKeyConsonant;
bool enableJIDARAKU = TRUE;
bool enableExtendedRAKU = TRUE;

bool isControl = FALSE;

bool isFunctionMode = FALSE;
bool isFunctionLeave = FALSE;

bool isCursorMode = FALSE;
bool isCursorLeave = FALSE;

int main( int argc, char *argv[] ){

   const TCHAR CLASS_NAME[] = "JIDARAKU";
   
   WNDCLASS wc = { };

   wc.lpfnWndProc = WindowProc;
   HINSTANCE hInstance;
   hInstance = GetModuleHandle( NULL );
   wc.hInstance = hInstance;
   wc.lpszClassName = CLASS_NAME;

   RegisterClass(&wc);


   hWnd = CreateWindowEx(
      //WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
      WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
      CLASS_NAME,
      "じだらく",
      WS_POPUP | WS_BORDER,
      0, 0, 0, 0,
      NULL,
      NULL,
      hInstance,
      NULL
      );

   if (hWnd == NULL){
      return -1;
   }

   MSG msg;
   while ( GetMessage(&msg, NULL, 0, 0) ) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   return 0;
}

LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch (uMsg) {

      case WM_CREATE:
         HookStart();
         break;

      case WM_DESTROY:
         HookEnd();
         PostQuitMessage(0);
         return 0;
   }

   return DefWindowProc( hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK HookProc(int nCode, WPARAM wp, LPARAM lp)
{
   if ( nCode != HC_ACTION ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   KBDLLHOOKSTRUCT *key;
   unsigned int keyCode, scanCode, keyFlags, keyChar;

   key = (KBDLLHOOKSTRUCT*)lp;
   
   keyCode = key->vkCode;
   scanCode = key->scanCode;
   keyFlags = key->flags;
   keyChar = MapVirtualKey( keyCode, MAPVK_VK_TO_CHAR );

   printf( "(%c), %u, %u, %u, ", keyChar, keyCode, scanCode, keyFlags );

   bool isKeyUp;
   isKeyUp = keyFlags & LLKHF_UP;
   if ( isKeyUp ){
      printf( "KeyUp " );
   }
   else{
      printf( "KeyDown " );
   }

   bool isInjected;
   isInjected = keyFlags & LLKHF_INJECTED;
   if( isInjected ){
      printf( "injected\n" );
   }
   else{
      printf( "detected\n" );
   }

  // Ctrlキー状態 
  // Ctrl + △ を Ctrl + □　ではなく □ 単打に入れ替えられるようにCtrl状態を内部管理する
   if( isInjected == FALSE ){
      if( isKeyUp ){
         if( keyCode == VK_OEM_PERIOD ){
            SendKeyUp( VK_RCONTROL );
            return TRUE;
         }
         else if( keyCode == 'X' ){
            SendKeyUp( VK_LCONTROL );
            return TRUE;
         }
      }
      else {
         if( keyCode == VK_OEM_PERIOD ){
            SendKeyDown( VK_RCONTROL );
            return TRUE;
         }
         else if( keyCode == 'X' ){
            SendKeyDown( VK_LCONTROL );
            return TRUE;
         }
      }
   }
   if ( keyCode == VK_CONTROL || keyCode == VK_LCONTROL || keyCode == VK_RCONTROL ){
      if ( isInjected ){
         return CallNextHookEx(hHook, nCode, wp, lp);
      }
      else {
         if( isKeyUp  ){
            isControl = FALSE;
            SendKeyUp( VK_CONTROL );
         }
         else {
            isControl = TRUE;
         }
         return TRUE;
      }
   }

   // Shiftキー状態
   if( isInjected == FALSE ){
      if( isKeyUp ){
         if( keyCode == 'Z' ){
            SendKeyUp( VK_LSHIFT );
            return TRUE;
         }
         else if( keyCode == VK_OEM_2 || keyCode == VK_UP ){
            SendKeyUp( VK_RSHIFT );
            return TRUE;
         }
      }
      else {
         if( keyCode == 'Z' ){
            SendKeyDown( VK_LSHIFT );
            return TRUE;
         }
         else if( keyCode == VK_OEM_2 || keyCode == VK_UP ){
            SendKeyDown( VK_RSHIFT );
            return TRUE;
         }
      }
   }
   bool isShift;
   isShift = GetAsyncKeyState( VK_SHIFT) & 0x8000;

   
   // Altキー状態
   if( isInjected == FALSE ){
      if( isKeyUp ){
         if( keyCode == VK_LEFT ){
            SendKeyUp( VK_RMENU );
            return TRUE;
         }
      }
      else {
         if( keyCode == VK_LEFT ){
            SendKeyDown( VK_RMENU );
            return TRUE;
         }
      }
   }
   bool isAlt;
   isAlt = GetAsyncKeyState( VK_MENU ) & 0x8000;

   // Spaceキー状態
   static bool isSpace = FALSE;
   //isSpace = GetAsyncKeyState( VK_SPACE) & 0x8000;

   HWND hForeground;
   hForeground = GetForegroundWindow();

   HWND hIME;
   hIME = ImmGetDefaultIMEWnd( hForeground );

   LRESULT imeStatus;
   imeStatus = SendMessage( hIME, WM_IME_CONTROL, 5, 0 );

   // スペース
   if( keyCode == VK_SPACE ){
      if( isInjected == FALSE ){
         if( isKeyUp  ){
            isSpace = FALSE;
            if( lastKeyCode == VK_SPACE ){
               SendKey( VK_SPACE );
            }
            return TRUE;
         }
         else {
            isSpace = TRUE;
            lastKeyCode = VK_SPACE;
            return TRUE;
         }
      }
      else {
         return FALSE;
      }
   }

   
   // キーアップ処理
   if( isKeyUp  ){
      if ( keyCode == 'V' && isInjected == FALSE ){
         if( isFunctionLeave == TRUE ){
            isFunctionMode = FALSE;
            isFunctionLeave = FALSE;
         }
         else{
            isFunctionMode = FALSE;
            //if( isShift == FALSE ){
            if ( lastKeyCode != VK_OEM_2 ){
                  SendKey( VK_OEM_2 ); // /? 
                  lastKeyCode = NULL;
            }
         }
         return TRUE;
      }
      else if ( keyCode == 'B' && isInjected == FALSE ){
         if( isCursorLeave == TRUE ){
            isCursorMode = FALSE;
            isCursorLeave = FALSE;
         }
         else{
            isCursorMode = FALSE;
            SendKey( VK_OEM_5 ); // \| 
         }
         return TRUE;
      }
      else if ( ( keyCode == VK_OEM_PA1 || keyCode == 0x16 || keyCode == 'B' ) && isInjected == FALSE ){
         if( isCursorLeave == TRUE ){
            isCursorMode = FALSE;
            isCursorLeave = FALSE;
         }
         else{
            isCursorMode = FALSE;
            //if( isShift == FALSE ){
            if ( lastKeyCode != 'N' ){
                  SendKey( 'N' );
                  SendKey( 'N' );
                  lastKeyCode = NULL;
            }
         }
         return TRUE;
      }
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   // Ctrlキーと同時押しのキーバインド設定
   // 設定のないキーはQWERTYで処理する
   if( isControl ) {
      if ( ( keyFlags & LLKHF_INJECTED ) == FALSE ){
         if ( keyCode == 'M' ){
            SendKey( VK_RETURN );
         }
         else if ( keyCode == 'H' ){
            SendKey( VK_BACK );
         }
         else {
            SendKey( VK_CONTROL, keyCode );
         }

         return TRUE;
      }
   }

   // Alt キーとの同時押しのキーバインド設定
   if( isAlt  ){
      // Alt + Q で QWERTY ⇔ じだらく 切り替え
      if( keyCode == 'Q' ){
         enableJIDARAKU = !enableJIDARAKU;
         return TRUE;
      }
      // Alt + X で 拡張処理の有効無効をトグル
      else if( keyCode == 'X' ){
         enableExtendedRAKU= !enableExtendedRAKU;
         return TRUE;
      }
   }

   // 無効状態のとき入れ替え処理しない
   if ( enableJIDARAKU == FALSE ){
      return CallNextHookEx(hHook, nCode, wp, lp);
   }
   
   // 挿入されたキーイベント
   if ( isInjected ){
      //子音キーが挿入されたら促音拡張処理に入る
      if( keyCode == 'B' ||
          keyCode == 'C' || 
          keyCode == 'D' || 
          keyCode == 'F' || 
          keyCode == 'G' || 
          keyCode == 'H' || 
          keyCode == 'J' || 
          keyCode == 'K' || 
          keyCode == 'L' || 
          keyCode == 'M' || 
          keyCode == 'N' || 
          keyCode == 'P' || 
          keyCode == 'Q' || 
          keyCode == 'R' || 
          keyCode == 'S' || 
          keyCode == 'T' || 
          keyCode == 'V' || 
          keyCode == 'W' || 
          keyCode == 'X' || 
          keyCode == 'Y' || 
          keyCode == 'Z' ) {


         // 拡張処理
         if( isLastKeyConsonant == TRUE ){
            // 同じ子音2連続は拡張処理しない
            if ( lastKeyCode == keyCode ){
               // NN は拡張処理せず拡張状態を終了する
               if ( lastKeyCode == 'N' && keyCode == 'N' ){
                  isLastKeyConsonant = FALSE;
               }
               // NN以外の同じ子音2連続は拡張処理処理しないが、拡張状態を継続する
               else {
                  isLastKeyConsonant = TRUE;
               }
            }
            // shift押下中は拡張処理しないが、各状態を継続する
            else if ( isShift ){
               //isLastKeyConsonant = FALSE;
            }
            // XN は拡張処理せず拡張状態を終了する
            else if ( lastKeyCode == 'X' && keyCode == 'N' ){
               isLastKeyConsonant = FALSE;
            }
            // 促音拡張処理
            // 最後に打たれたキーが'S'であれば、削除して"っ（ltu）"に置き換える
            // e.q. sk → ltuk
            else if( lastKeyCode == 'S' ){
               if( keyCode != 'Y' ){
                  SendKey( VK_BACK );
                  SendKey( 'L' );
                  SendKey( 'T' );
                  SendKey( 'U' );
                  isLastKeyConsonant = TRUE;
               }
            }
         }
         // 子音が挿入されたら拡張状態に入る
         else {
            if( enableExtendedRAKU ){
               isLastKeyConsonant = TRUE;
            }
         }
      }
      else if( keyCode == 'A' ||
               keyCode == 'E' || 
               keyCode == 'I' || 
               keyCode == 'O' || 
               keyCode == 'U' ) {
         /*
         if( lastKeyCode == 'X' ){
            SendKey( 'T' );
            SendKey( 'U' );
            SendKey( 'S' );
         }
         */
         isLastKeyConsonant = FALSE;
      }
      else {
         // 子音以外のキーが挿入されたら拡張レイヤーから抜ける
         isLastKeyConsonant = FALSE;
      }

      //最後に挿入されたキーを記録
      lastKeyCode = keyCode;

      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   // キー入れ替え処理
   //最上段
   if ( keyCode == VK_OEM_MINUS ){ // -_ 
      SendKey( 'X' );
   }
   else if ( keyCode == VK_OEM_PLUS ){ // =+
      SendKey( 'C' );
   }
   //上段左
   else if ( keyCode == 'Q' ){
      SendKey( VK_OEM_7 ); // '"
   }
   else if ( keyCode == 'W' ){
      if( lastKeyCode == 'Y' || lastKeyCode == 'G' ){
         SendKey( VK_BACK );
         SendKey( VK_OEM_4 ); // [{
      }
      else if( lastKeyCode == 'F' ){
         SendKey( VK_BACK );
         SendKey( VK_OEM_MINUS );
      }
      else{
         SendKey( VK_OEM_COMMA ); // ,<
         
      }
   }
   else if ( keyCode == 'E' ){
      if( lastKeyCode == 'Y' || lastKeyCode == 'G' ){
         SendKey( VK_BACK );
         SendKey( VK_OEM_6 ); // ]}
      }
      else if( lastKeyCode == 'F' ){
         SendKey( VK_BACK );
         SendKey( VK_OEM_PLUS);
      }
      else{
         SendKey( VK_OEM_PERIOD ); // .>
      }
   }
   else if ( keyCode == 'R' ){
      SendKey( VK_OEM_MINUS); // -_
   }
   else if ( keyCode == 'T' ){
      SendKey( VK_OEM_PLUS ); // =+
   }
   //上段右
   else if ( keyCode == 'Y' ){
      SendKey( 'Y' );
   }
   else if ( keyCode == 'U' ){
      SendKey( 'D' );
   }
   else if ( keyCode == 'I' ){
      if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'P' );
      }
      else {
         if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_PRIOR );
         }
         else {
            SendKey( 'G' );
         }
      }
   }
   else if ( keyCode == 'O' ){
      if( isLastKeyConsonant && lastKeyCode == 'G'  ){
         SendKey( VK_BACK );
         SendKey( 'V' );
      }
      else {
         if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_NEXT );
         }
         else {
            SendKey( 'F' );
         }
      }
   }
   else if ( keyCode == 'P' ){
      SendKey( 'P' );
   }
   else if ( keyCode == VK_OEM_4 ){ // [{
      SendKey( 'B' );
   }
   else if ( keyCode == VK_OEM_6 ){ // ]}
      SendKey( 'V' );
   }
   //中段左
   else if ( keyCode == 'A' ){
      if( isSpace == TRUE ){
         SendKey( '1' );
      }
      else {
         SendKey( 'A' );
      }
   }
   else if ( keyCode == 'S' ){
      if( isSpace == TRUE ){
         SendKey( '2' );
      }
      else {
         SendKey( 'O' );
      }
   }
   else if ( keyCode == 'D' ){
      if( isSpace == TRUE ){
         SendKey( '3' );
      }
      else {
         SendKey( 'E' );
      }
   }
   else if ( keyCode == 'F' ){
      if( isSpace == TRUE ){
         SendKey( '4' );
      }
      else {
         SendKey( 'U' );
      }
   }
   else if ( keyCode == 'G' ){
      if( isSpace == TRUE ){
         SendKey( '5' );
      }
      else {
         SendKey( 'I' );
      }
   }
   //中段右
   else if ( keyCode == 'H' ){
      
      if( isFunctionMode == TRUE ){
         isFunctionLeave = TRUE;
         SendKey( VK_BACK );
      }
      else if( isCursorMode == TRUE ){
         isCursorLeave = TRUE;
         SendKey( VK_LEFT );
      }
      else if( isSpace == TRUE ){
         SendKey( '6' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'G'  ){
         SendKey( VK_BACK );
         SendKey( 'B' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'W' );
      }
      else {
         SendKey( 'H' );
      }
   }
   else if ( keyCode == 'J' ){
      if( isSpace == TRUE ){
         SendKey( '7' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'G'  ){
         SendKey( VK_BACK );
         SendKey( 'D' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'X' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'Y'  ){
         SendKey( VK_BACK );
         SendKey( 'X' );
      }
      else {
         if( isFunctionMode == TRUE ){
            isFunctionLeave = TRUE;
            SendKey( VK_TAB );
         }
         else if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_DOWN );
         }
         else {
            SendKey( 'T' );
         }
      }
   }
   else if ( keyCode == 'K' ){
      if( isSpace == TRUE ){
         SendKey( '8' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'Q' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'Y'  ){
         SendKey( VK_BACK );
         SendKey( 'Q' );
      }
      else {
         if( isFunctionMode == TRUE ){
            isFunctionLeave = TRUE;
            SendKey( VK_ESCAPE );
         }
         else if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_UP );
         }
         else {
            SendKey( 'K' );
         }
      }
   }
   else if ( keyCode == 'L' ){
      if( isSpace == TRUE ){
         SendKey( '9' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'L' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'Y'  ){
         SendKey( VK_BACK );
         SendKey( 'L' );
      }
      else {
         if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_RIGHT );
         }
         else {
            SendKey( 'R' );
         }
      }
   }
   else if ( keyCode == VK_OEM_1 ){ // ;:
      if( isSpace == TRUE ){
         SendKey( '0' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'G'  ){
         SendKey( VK_BACK );
         SendKey( 'Z' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'C' );
      }
      else if( isLastKeyConsonant && lastKeyCode == 'Y'  ){
         SendKey( VK_BACK );
         SendKey( 'C' );
      }
      else {
         if( isFunctionMode == TRUE ){
            isFunctionLeave = TRUE;
            SendKey( VK_CAPITAL );
         }
         else {
            SendKey( 'S' );
         }
      }
   }
   else if ( keyCode == VK_OEM_7 ){ // '"
      SendKey( VK_OEM_MINUS );
   }
   else if ( keyCode == VK_OEM_5 ){ // \|
      SendKey( 'L');
   }
   //下段左
   else if ( keyCode == 'Z' ){
      SendKey( VK_OEM_1 ); // ;:
   }
   else if ( keyCode == 'X' ){
      SendKey( VK_OEM_4 ); // [{
   }
   else if ( keyCode == 'C' ){
      SendKey( VK_OEM_6 ); // ]}
   }
   else if ( keyCode == 'V' ){
      if( isShift == TRUE ){
         isFunctionMode = FALSE;
         isFunctionLeave = TRUE;
         SendKey( VK_OEM_2 ); // /?
      }
      else if( isFunctionMode == FALSE ){
         isFunctionMode = TRUE;
      }
      else {
         isFunctionMode = FALSE;
         SendKey( VK_OEM_2 ); // /?
      }
   }
   else if ( keyCode == 'B' ){
      isCursorMode = TRUE;
      //SendKey( VK_OEM_5 ); // \| 
   }
   //下段右
   else if ( keyCode == 'N' ){
      if( isFunctionMode == TRUE ){
         isFunctionLeave = TRUE;
         SendKey( VK_DELETE );
      }
      else {
         if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_HOME );
         }
         else {
            SendKey( 'N' );
         }
      }
   }
   else if ( keyCode == 'M' ){
      if( isFunctionMode == TRUE ){
         isFunctionLeave = TRUE;
         SendKey( VK_RETURN );
      }
      else{
         if( isCursorMode == TRUE ){
            isCursorLeave = TRUE;
            SendKey( VK_END );
         }
         else {
            SendKey( 'M' );
         }
      }
   }
   else if ( keyCode == VK_OEM_COMMA ){ // ,<
      if( lastKeyCode == 'J'  ){
         SendKey( VK_BACK );
         SendKey( 'V' );
      }
      else {
         SendKey( 'W' );
      }
   }
   else if ( keyCode == VK_OEM_PERIOD ){ // .>
      SendKey( 'J' );
   }
   else if ( keyCode == VK_OEM_2 ){ // /?
      SendKey( 'Z' );
   }
   else if ( keyCode == VK_OEM_2 ){ // /?
      SendKey( 'Z' );
   }
   else if ( keyCode == 0x16){ // /?
      SendKey( 'N' );
      SendKey( 'N' );
   }
   else if ( keyCode == VK_OEM_PA1 ){ //無変換
      if( lastKeyCode == VK_SPACE ){
         SendKeyDown( VK_SHIFT );
      }
      else {
         isCursorMode = TRUE;
      }
   }
   else if ( keyCode == 0x1A ) {
      if( lastKeyCode == 'F'  ){
         SendKey( VK_BACK );
         SendKey( 'J' );
      }
      else {
         SendKey( 'Y' );
      }
   }
   else if ( keyCode == 255  ) {
      if( scanCode == 121 ){ //変換
         if( lastKeyCode == 'F'  ){
            SendKey( VK_BACK );
            SendKey( 'J' );
         }
         else {
            SendKey( 'Y' );
         }
      }
      else if( scanCode == 112 ){ //カタカナひらがなローマ字
      }
   }
   //入れ替え対象外
   else {
      lastKeyCode = keyCode;
      isLastKeyConsonant = FALSE;
      return CallNextHookEx(hHook, nCode, wp, lp);
   }

   return TRUE; 
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wp, LPARAM lp)
{

   if ( nCode != HC_ACTION ){
      return CallNextHookEx( hHook, nCode, wp, lp);
   }

   if( isControl ){
      SendKeyDown( VK_CONTROL );
   }

   return CallNextHookEx( hMouseHook, nCode, wp, lp);
}

void HookStart()
{
   printf( "keyChar, keyCode, scanCode, keyFlags, eventType\n" );
   hHook = SetWindowsHookEx( WH_KEYBOARD_LL, HookProc, hInst, 0);
   hMouseHook = SetWindowsHookEx( WH_MOUSE_LL, MouseHookProc, hInst, 0);
}

void HookEnd()
{
   UnhookWindowsHookEx(hHook);
   UnhookWindowsHookEx(hMouseHook);
}


char SendKey( char keyCode ){

   keybd_event( keyCode, NULL, NULL, NULL );
   keybd_event( keyCode, NULL, KEYEVENTF_KEYUP, NULL );

   return keyCode;
}

char SendKeyDown( char keyCode ){

   keybd_event( keyCode, NULL, NULL, NULL );

   return keyCode;
}
char SendKeyUp( char keyCode ){

   keybd_event( keyCode, NULL, KEYEVENTF_KEYUP, NULL );

   return keyCode;
}

char SendKey( char modifierCode, char keyCode ){

   keybd_event( modifierCode, NULL, NULL, NULL );

   keybd_event( keyCode, NULL, NULL, NULL );
   keybd_event( keyCode, NULL, KEYEVENTF_KEYUP, NULL );

   keybd_event( modifierCode, NULL, KEYEVENTF_KEYUP, NULL );

   return keyCode;
}

