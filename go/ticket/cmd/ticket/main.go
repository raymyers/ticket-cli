package main

import (
	"os"

	"github.com/raymyers/ticket-cli/go/ticket/internal/cli"
)

func main() {
	exitCode := cli.Run(os.Args[1:])
	os.Exit(exitCode)
}
