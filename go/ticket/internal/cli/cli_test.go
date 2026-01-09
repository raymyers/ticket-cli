package cli

import (
	"testing"
)

func TestRun_Help(t *testing.T) {
	tests := []struct {
		name string
		args []string
		want int
	}{
		{"no args", []string{}, 0},
		{"help flag", []string{"--help"}, 0},
		{"help command", []string{"help"}, 0},
		{"-h flag", []string{"-h"}, 0},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			got := Run(tt.args)
			if got != tt.want {
				t.Errorf("Run() = %v, want %v", got, tt.want)
			}
		})
	}
}

func TestRun_Create(t *testing.T) {
	got := Run([]string{"create"})
	if got != 0 {
		t.Errorf("Run(create) = %v, want 0", got)
	}
}

func TestRun_UnknownCommand(t *testing.T) {
	got := Run([]string{"unknown"})
	if got != 1 {
		t.Errorf("Run(unknown) = %v, want 1", got)
	}
}
