package main

import (
	"fmt"
	"github.com/mistree/AssistiveReader"
)

func main() {
	Baidu.Init()
	fmt.Println(Baidu.Translate("にほんご"))
}
