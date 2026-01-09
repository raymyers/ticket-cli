package cli

import (
	"bytes"
	"crypto/sha256"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

const ticketsDir = ".tickets"

// Run executes the CLI with the given arguments and returns an exit code.
func Run(args []string) int {
	if len(args) == 0 || args[0] == "-h" || args[0] == "--help" || args[0] == "help" {
		printHelp()
		return 0
	}

	command := args[0]
	commandArgs := args[1:]

	switch command {
	case "create":
		return cmdCreate(commandArgs)
	case "query":
		return cmdQuery(commandArgs)
	default:
		fmt.Println("Ticket CLI - Go port (work in progress)")
		fmt.Printf("Command not yet implemented: %s\n", command)
		return 1
	}
}

func printHelp() {
	fmt.Print(`tk - minimal ticket system with dependency tracking

Usage: tk <command> [args]

Commands:
  create [title] [options] Create ticket, prints ID
    -d, --description      Description text
    --design               Design notes
    --acceptance           Acceptance criteria
    -t, --type             Type (bug|feature|task|epic|chore) [default: task]
    -p, --priority         Priority 0-4, 0=highest [default: 2]
    -a, --assignee         Assignee [default: git user.name]
    --external-ref         External reference (e.g., gh-123, JIRA-456)
    --parent               Parent ticket ID
  start <id>               Set status to in_progress
  close <id>               Set status to closed
  reopen <id>              Set status to open
  status <id> <status>     Update status (open|in_progress|closed)
  dep <id> <dep-id>        Add dependency (id depends on dep-id)
  dep tree [--full] <id>   Show dependency tree (--full disables dedup)
  undep <id> <dep-id>      Remove dependency
  link <id> <id> [id...]   Link tickets together (symmetric)
  unlink <id> <target-id>  Remove link between tickets
  ls [--status=X]          List tickets
  ready                    List open/in_progress tickets with deps resolved
  blocked                  List open/in_progress tickets with unresolved deps
  closed [--limit=N]       List recently closed tickets (default 20, by mtime)
  show <id>                Display ticket
  edit <id>                Open ticket in $EDITOR
  add-note <id> [text]     Append timestamped note (or pipe via stdin)
  query [jq-filter]        Output tickets as JSON, optionally filtered
  migrate-beads            Import tickets from .beads/issues.jsonl

Tickets stored as markdown files in .tickets/
Supports partial ID matching (e.g., 'tk show 5c4' matches 'nw-5c46')
`)
}

func generateID() string {
	cwd, err := os.Getwd()
	if err != nil {
		cwd = "ticket"
	}

	dirName := filepath.Base(cwd)

	// Extract first letter of each hyphenated/underscored segment
	segments := strings.FieldsFunc(dirName, func(r rune) bool {
		return r == '-' || r == '_'
	})

	var prefix string
	for _, s := range segments {
		if len(s) > 0 {
			prefix += string(s[0])
		}
	}

	// Fallback to first 3 chars if no segments
	if prefix == "" {
		if len(dirName) >= 3 {
			prefix = dirName[:3]
		} else {
			prefix = dirName
		}
	}

	// 4-char hash from timestamp + PID for entropy
	entropy := fmt.Sprintf("%d%d", os.Getpid(), time.Now().UTC().Unix())
	hash := sha256.Sum256([]byte(entropy))
	hashStr := fmt.Sprintf("%x", hash)[:4]

	return fmt.Sprintf("%s-%s", prefix, hashStr)
}

func isoDate() string {
	return time.Now().UTC().Format("2006-01-02T15:04:05Z")
}

func ensureDir() error {
	return os.MkdirAll(ticketsDir, 0755)
}

func getGitUserName() string {
	cmd := exec.Command("git", "config", "user.name")
	output, err := cmd.Output()
	if err != nil {
		return ""
	}
	return strings.TrimSpace(string(output))
}

func cmdCreate(args []string) int {
	if err := ensureDir(); err != nil {
		fmt.Fprintf(os.Stderr, "Error creating tickets directory: %v\n", err)
		return 1
	}

	title := ""
	description := ""
	design := ""
	acceptance := ""
	priority := 2
	issueType := "task"
	assignee := getGitUserName()
	externalRef := ""
	parent := ""

	// Parse args
	i := 0
	for i < len(args) {
		arg := args[i]

		if arg == "-d" || arg == "--description" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			description = args[i+1]
			i += 2
		} else if arg == "--design" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			design = args[i+1]
			i += 2
		} else if arg == "--acceptance" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			acceptance = args[i+1]
			i += 2
		} else if arg == "-p" || arg == "--priority" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			p, err := strconv.Atoi(args[i+1])
			if err != nil {
				fmt.Fprintf(os.Stderr, "Error: invalid priority value: %s\n", args[i+1])
				return 1
			}
			priority = p
			i += 2
		} else if arg == "-t" || arg == "--type" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			issueType = args[i+1]
			i += 2
		} else if arg == "-a" || arg == "--assignee" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			assignee = args[i+1]
			i += 2
		} else if arg == "--external-ref" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			externalRef = args[i+1]
			i += 2
		} else if arg == "--parent" {
			if i+1 >= len(args) {
				fmt.Fprintf(os.Stderr, "Error: %s requires an argument\n", arg)
				return 1
			}
			parent = args[i+1]
			i += 2
		} else if strings.HasPrefix(arg, "-") {
			fmt.Fprintf(os.Stderr, "Unknown option: %s\n", arg)
			return 1
		} else {
			title = arg
			i++
		}
	}

	if title == "" {
		title = "Untitled"
	}

	ticketID := generateID()
	filePath := filepath.Join(ticketsDir, ticketID+".md")
	now := isoDate()

	// Build content
	var contentParts []string
	contentParts = append(contentParts, "---")
	contentParts = append(contentParts, fmt.Sprintf("id: %s", ticketID))
	contentParts = append(contentParts, "status: open")
	contentParts = append(contentParts, "deps: []")
	contentParts = append(contentParts, "links: []")
	contentParts = append(contentParts, fmt.Sprintf("created: %s", now))
	contentParts = append(contentParts, fmt.Sprintf("type: %s", issueType))
	contentParts = append(contentParts, fmt.Sprintf("priority: %d", priority))
	if assignee != "" {
		contentParts = append(contentParts, fmt.Sprintf("assignee: %s", assignee))
	}
	if externalRef != "" {
		contentParts = append(contentParts, fmt.Sprintf("external-ref: %s", externalRef))
	}
	if parent != "" {
		contentParts = append(contentParts, fmt.Sprintf("parent: %s", parent))
	}
	contentParts = append(contentParts, "---")
	contentParts = append(contentParts, fmt.Sprintf("# %s", title))
	contentParts = append(contentParts, "")

	if description != "" {
		contentParts = append(contentParts, description)
		contentParts = append(contentParts, "")
	}

	if design != "" {
		contentParts = append(contentParts, "## Design")
		contentParts = append(contentParts, "")
		contentParts = append(contentParts, design)
		contentParts = append(contentParts, "")
	}

	if acceptance != "" {
		contentParts = append(contentParts, "## Acceptance Criteria")
		contentParts = append(contentParts, "")
		contentParts = append(contentParts, acceptance)
		contentParts = append(contentParts, "")
	}

	content := strings.Join(contentParts, "\n")

	if err := os.WriteFile(filePath, []byte(content), 0644); err != nil {
		fmt.Fprintf(os.Stderr, "Error writing ticket file: %v\n", err)
		return 1
	}

	fmt.Println(ticketID)
	return 0
}

func cmdQuery(args []string) int {
	// Check if tickets directory exists
	if _, err := os.Stat(ticketsDir); os.IsNotExist(err) {
		return 0
	}

	var filter string
	if len(args) > 0 {
		filter = args[0]
	}

	// Read all ticket files
	files, err := filepath.Glob(filepath.Join(ticketsDir, "*.md"))
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading tickets directory: %v\n", err)
		return 1
	}

	var jsonLines []string

	for _, file := range files {
		ticket, err := parseTicket(file)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error parsing ticket %s: %v\n", file, err)
			continue
		}

		jsonData, err := json.Marshal(ticket)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error marshaling ticket %s: %v\n", file, err)
			continue
		}

		jsonLines = append(jsonLines, string(jsonData))
	}

	jsonOutput := strings.Join(jsonLines, "\n")
	if jsonOutput == "" {
		return 0
	}

	// Add newline at the end if there's content
	jsonOutput += "\n"

	// If filter is provided, pipe through jq
	if filter != "" {
		cmd := exec.Command("jq", "-c", "select("+filter+")")
		cmd.Stdin = strings.NewReader(jsonOutput)
		var stdout, stderr bytes.Buffer
		cmd.Stdout = &stdout
		cmd.Stderr = &stderr

		if err := cmd.Run(); err != nil {
			fmt.Fprintf(os.Stderr, "jq error: %s\n", stderr.String())
			return 1
		}

		output := stdout.String()
		if output != "" {
			fmt.Print(output)
		}
	} else {
		fmt.Print(jsonOutput)
	}

	return 0
}

func parseTicket(filePath string) (map[string]interface{}, error) {
	content, err := os.ReadFile(filePath)
	if err != nil {
		return nil, err
	}

	lines := strings.Split(string(content), "\n")

	// Find frontmatter boundaries
	var frontmatterStart, frontmatterEnd int
	frontmatterCount := 0
	for i, line := range lines {
		if strings.TrimSpace(line) == "---" {
			frontmatterCount++
			if frontmatterCount == 1 {
				frontmatterStart = i
			} else if frontmatterCount == 2 {
				frontmatterEnd = i
				break
			}
		}
	}

	if frontmatterCount < 2 {
		return nil, fmt.Errorf("invalid frontmatter in %s", filePath)
	}

	ticket := make(map[string]interface{})

	// Parse frontmatter
	for i := frontmatterStart + 1; i < frontmatterEnd; i++ {
		line := lines[i]
		if strings.TrimSpace(line) == "" {
			continue
		}

		parts := strings.SplitN(line, ":", 2)
		if len(parts) != 2 {
			continue
		}

		key := strings.TrimSpace(parts[0])
		value := strings.TrimSpace(parts[1])

		// Parse arrays
		if strings.HasPrefix(value, "[") && strings.HasSuffix(value, "]") {
			arrayContent := strings.TrimSpace(value[1 : len(value)-1])
			if arrayContent == "" {
				ticket[key] = []string{}
			} else {
				// Split by comma and trim whitespace
				items := strings.Split(arrayContent, ",")
				var trimmedItems []string
				for _, item := range items {
					trimmed := strings.TrimSpace(item)
					if trimmed != "" {
						trimmedItems = append(trimmedItems, trimmed)
					}
				}
				ticket[key] = trimmedItems
			}
		} else if key == "priority" {
			// Parse priority as integer
			priority, err := strconv.Atoi(value)
			if err == nil {
				ticket[key] = priority
			} else {
				ticket[key] = value
			}
		} else {
			ticket[key] = value
		}
	}

	return ticket, nil
}
