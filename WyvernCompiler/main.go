package main

import (
	"flag"
	"fmt"
	"os"
)

func main() {
	flag.Parse()
	if len(flag.Args()) == 0 {
		fmt.Fprintln(os.Stderr, "usage: WyvernCompiler <manifest>")
		flag.PrintDefaults()
		return
	}
	srcFile, _ := os.ReadFile(flag.Arg(0))
	nodes, err := Parse(string(srcFile))
	if err != nil {
		panic(err)
	}
	for _, node := range nodes {
		println(node.String())
	}
}
