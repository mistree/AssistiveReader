// AssistiveReader
package main

import (
	"fmt"
	"github.com/AllenDang/gform"
	"github.com/AllenDang/w32"
	"github.com/atotto/clipboard"
	"github.com/mistree/AssistiveReader"
	"os"
	"syscall"
	"unsafe"
)

var (
	ReaderBar = syscall.MustLoadDLL("ReaderBar.dll")
	_Init     = ReaderBar.MustFindProc("Init")
	_DeInit   = ReaderBar.MustFindProc("DeInit")
	_Show     = ReaderBar.MustFindProc("Show")
	_Display  = ReaderBar.MustFindProc("Display")
	_Clear    = ReaderBar.MustFindProc("Clear")
	Win       = gform.NewForm(nil)
)

func Init() {
	_Init.Call(uintptr(Win.Handle()))
}

func DeInit() {
	_DeInit.Call()
}

func Clear() {
	_Clear.Call()
}

func Show(Visible int) {
	_Show.Call(uintptr(Visible))
}

func Display(Text string) {
	_Display.Call(uintptr(unsafe.Pointer(syscall.StringToUTF16Ptr(Text))))
}

func main() {
	fmt.Println(len(os.Args), os.Args)
	Baidu.Init()
	Init()
	Show(1)
	gform.Init()
	w32.AddClipboardFormatListener(Win.Handle())
	Win.Bind(w32.WM_CLIPBOARDUPDATE, ClipboardUpdate)
	Win.Bind(w32.WM_DESTROY, Quit)
	gform.RunMainLoop()
	DeInit()
}

func ClipboardUpdate(arg *gform.EventArg) {
	text, _ := clipboard.ReadAll()
	Display(Baidu.Translate(text))
}

func Quit(arg *gform.EventArg) {
	Win.Close()
}
