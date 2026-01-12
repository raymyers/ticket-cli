#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <openssl/sha.h>

#define VERSION "0.1.0"
#define TICKETS_DIR ".tickets"
#define MAX_PATH 1024
#define MAX_MATCHES 100
#define MAX_LINKS 100
#define MAX_TICKETS 100

static void print_usage(const char *program_name) {
    printf("Ticket CLI - C Implementation\n");
    printf("Version: %s\n\n", VERSION);
    printf("Usage: %s <command> [arguments]\n\n", program_name);
    printf("Commands:\n");
    printf("  create [title]              Create a new ticket\n");
    printf("  show <id>                   Show ticket details\n");
    printf("  list                        List all tickets\n");
    printf("  status <id> <status>        Update ticket status\n");
    printf("  start <id>                  Set status to in_progress\n");
    printf("  close <id>                  Set status to closed\n");
    printf("  reopen <id>                 Set status to open\n");
    printf("  dep <id> <dep-id>           Add dependency\n");
    printf("  dep tree <id>               Show dependency tree\n");
    printf("  link <id> <id> [id...]      Add symmetric links\n");
    printf("  unlink <id> <id>            Remove symmetric link\n");
    printf("  edit <id>                   Edit ticket in $EDITOR\n");
    printf("  add-note <id> <note>        Add note to ticket\n");
    printf("  query [options]             Query tickets (JSON output)\n");
    printf("  help                        Show this help message\n");
    printf("  version                     Show version information\n");
    printf("\n");
    printf("For more information, see the documentation.\n");
}

static void print_version(void) {
    printf("ticket-cli (C implementation) version %s\n", VERSION);
}

static int resolve_ticket_id(const char *ticket_id, char *resolved_path, size_t path_size) {
    char exact_path[MAX_PATH];
    struct stat st;
    
    snprintf(exact_path, sizeof(exact_path), "%s/%s.md", TICKETS_DIR, ticket_id);
    
    if (stat(exact_path, &st) == 0 && S_ISREG(st.st_mode)) {
        snprintf(resolved_path, path_size, "%s", exact_path);
        return 0;
    }
    
    DIR *dir = opendir(TICKETS_DIR);
    if (dir == NULL) {
        fprintf(stderr, "Error: cannot open tickets directory\n");
        return 1;
    }
    
    char matches[MAX_MATCHES][MAX_PATH];
    int match_count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 3, ".md") != 0) {
            continue;
        }
        
        if (strstr(entry->d_name, ticket_id) != NULL) {
            if (match_count < MAX_MATCHES) {
                snprintf(matches[match_count], MAX_PATH, "%s/%s", TICKETS_DIR, entry->d_name);
                match_count++;
            }
        }
    }
    
    closedir(dir);
    
    if (match_count == 0) {
        fprintf(stderr, "Error: ticket '%s' not found\n", ticket_id);
        return 1;
    }
    
    if (match_count > 1) {
        fprintf(stderr, "Error: ambiguous ID '%s' matches multiple tickets\n", ticket_id);
        return 1;
    }
    
    snprintf(resolved_path, path_size, "%s", matches[0]);
    return 0;
}

static int cmd_not_implemented(const char *command) {
    fprintf(stderr, "Command '%s' not yet implemented\n", command);
    return 1;
}

static void get_iso_date(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", utc);
}

static void generate_ticket_id(char *buffer, size_t size) {
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(buffer, size, "tck-0000");
        return;
    }
    
    char *dir_name = strrchr(cwd, '/');
    if (dir_name == NULL) {
        dir_name = cwd;
    } else {
        dir_name++;
    }
    
    char prefix[32] = {0};
    int prefix_len = 0;
    int in_segment = 0;
    
    for (const char *p = dir_name; *p && prefix_len < 10; p++) {
        if (*p == '-' || *p == '_' || *p == ' ') {
            in_segment = 0;
        } else if (!in_segment) {
            prefix[prefix_len++] = *p;
            in_segment = 1;
        }
    }
    
    if (prefix_len == 0) {
        strncpy(prefix, dir_name, 3);
        prefix[3] = '\0';
    } else {
        prefix[prefix_len] = '\0';
    }
    
    char entropy[256];
    snprintf(entropy, sizeof(entropy), "%d%ld", getpid(), (long)time(NULL));
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)entropy, strlen(entropy), hash);
    
    snprintf(buffer, size, "%s-%02x%02x", prefix, hash[0], hash[1]);
}

static void ensure_tickets_dir(void) {
    struct stat st = {0};
    if (stat(TICKETS_DIR, &st) == -1) {
        mkdir(TICKETS_DIR, 0755);
    }
}

static int cmd_create(int argc, char *argv[]) {
    ensure_tickets_dir();
    
    char title[1024] = "";
    char description[4096] = "";
    char design[4096] = "";
    char acceptance[4096] = "";
    int priority = 2;
    char issue_type[64] = "task";
    char assignee[256] = "";
    char external_ref[256] = "";
    char parent[64] = "";
    
    FILE *git_cmd = popen("git config user.name 2>/dev/null", "r");
    if (git_cmd != NULL) {
        if (fgets(assignee, sizeof(assignee), git_cmd) != NULL) {
            size_t len = strlen(assignee);
            if (len > 0 && assignee[len-1] == '\n') {
                assignee[len-1] = '\0';
            }
        }
        pclose(git_cmd);
    }
    
    int i = 1;
    while (i < argc) {
        const char *arg = argv[i];
        if (strcmp(arg, "-d") == 0 || strcmp(arg, "--description") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(description, argv[i + 1], sizeof(description) - 1);
            i += 2;
        } else if (strcmp(arg, "--design") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(design, argv[i + 1], sizeof(design) - 1);
            i += 2;
        } else if (strcmp(arg, "--acceptance") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(acceptance, argv[i + 1], sizeof(acceptance) - 1);
            i += 2;
        } else if (strcmp(arg, "-p") == 0 || strcmp(arg, "--priority") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            priority = atoi(argv[i + 1]);
            i += 2;
        } else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--type") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(issue_type, argv[i + 1], sizeof(issue_type) - 1);
            i += 2;
        } else if (strcmp(arg, "-a") == 0 || strcmp(arg, "--assignee") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(assignee, argv[i + 1], sizeof(assignee) - 1);
            i += 2;
        } else if (strcmp(arg, "--external-ref") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(external_ref, argv[i + 1], sizeof(external_ref) - 1);
            i += 2;
        } else if (strcmp(arg, "--parent") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires an argument\n", arg);
                return 1;
            }
            strncpy(parent, argv[i + 1], sizeof(parent) - 1);
            i += 2;
        } else if (arg[0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", arg);
            return 1;
        } else {
            strncpy(title, arg, sizeof(title) - 1);
            i++;
        }
    }
    
    if (strlen(title) == 0) {
        strcpy(title, "Untitled");
    }
    
    char ticket_id[64];
    generate_ticket_id(ticket_id, sizeof(ticket_id));
    
    char file_path[MAX_PATH];
    snprintf(file_path, sizeof(file_path), "%s/%s.md", TICKETS_DIR, ticket_id);
    
    char now[32];
    get_iso_date(now, sizeof(now));
    
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot create ticket file\n");
        return 1;
    }
    
    fprintf(file, "---\n");
    fprintf(file, "id: %s\n", ticket_id);
    fprintf(file, "status: open\n");
    fprintf(file, "deps: []\n");
    fprintf(file, "links: []\n");
    fprintf(file, "created: %s\n", now);
    fprintf(file, "type: %s\n", issue_type);
    fprintf(file, "priority: %d\n", priority);
    if (strlen(assignee) > 0) {
        fprintf(file, "assignee: %s\n", assignee);
    }
    if (strlen(external_ref) > 0) {
        fprintf(file, "external-ref: %s\n", external_ref);
    }
    if (strlen(parent) > 0) {
        fprintf(file, "parent: %s\n", parent);
    }
    fprintf(file, "---\n");
    fprintf(file, "# %s\n\n", title);
    
    if (strlen(description) > 0) {
        fprintf(file, "%s\n\n", description);
    }
    
    if (strlen(design) > 0) {
        fprintf(file, "## Design\n\n%s\n\n", design);
    }
    
    if (strlen(acceptance) > 0) {
        fprintf(file, "## Acceptance Criteria\n\n%s\n\n", acceptance);
    }
    
    fclose(file);
    
    printf("%s\n", ticket_id);
    return 0;
}

static int cmd_show(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    
    char resolved_path[MAX_PATH];
    if (resolve_ticket_id(argv[1], resolved_path, sizeof(resolved_path)) != 0) {
        return 1;
    }
    
    FILE *file = fopen(resolved_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
    }
    
    fclose(file);
    return 0;
}

static int cmd_list(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    return cmd_not_implemented("list");
}

static int cmd_status(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: ticket ID and status required\n");
        return 1;
    }
    
    char resolved_path[MAX_PATH];
    if (resolve_ticket_id(argv[1], resolved_path, sizeof(resolved_path)) != 0) {
        return 1;
    }
    
    const char *new_status = argv[2];
    
    FILE *file = fopen(resolved_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char temp_path[MAX_PATH];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", resolved_path);
    FILE *temp_file = fopen(temp_path, "w");
    if (temp_file == NULL) {
        fclose(file);
        fprintf(stderr, "Error: cannot write temp file\n");
        return 1;
    }
    
    char line[1024];
    int in_frontmatter = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            fprintf(temp_file, "%s", line);
            in_frontmatter = !in_frontmatter;
            continue;
        }
        
        if (in_frontmatter && strncmp(line, "status:", 7) == 0) {
            fprintf(temp_file, "status: %s\n", new_status);
        } else {
            fprintf(temp_file, "%s", line);
        }
    }
    
    fclose(file);
    fclose(temp_file);
    
    if (rename(temp_path, resolved_path) != 0) {
        fprintf(stderr, "Error: cannot update ticket file\n");
        unlink(temp_path);
        return 1;
    }
    
    return 0;
}

static int cmd_start(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("start");
}

static int cmd_close(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("close");
}

static int cmd_reopen(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("reopen");
}

static int cmd_dep(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: ticket ID and dependency ID required\n");
        return 1;
    }
    
    char resolved_path[MAX_PATH];
    if (resolve_ticket_id(argv[1], resolved_path, sizeof(resolved_path)) != 0) {
        return 1;
    }
    
    char dep_path[MAX_PATH];
    if (resolve_ticket_id(argv[2], dep_path, sizeof(dep_path)) != 0) {
        return 1;
    }
    
    char dep_id[MAX_PATH];
    const char *basename = strrchr(dep_path, '/');
    if (basename != NULL) {
        basename++;
    } else {
        basename = dep_path;
    }
    snprintf(dep_id, sizeof(dep_id), "%.*s", (int)(strlen(basename) - 3), basename);
    
    FILE *file = fopen(resolved_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char temp_path[MAX_PATH];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", resolved_path);
    FILE *temp_file = fopen(temp_path, "w");
    if (temp_file == NULL) {
        fclose(file);
        fprintf(stderr, "Error: cannot write temp file\n");
        return 1;
    }
    
    char line[1024];
    int in_frontmatter = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            fprintf(temp_file, "%s", line);
            in_frontmatter = !in_frontmatter;
            continue;
        }
        
        if (in_frontmatter && strncmp(line, "deps:", 5) == 0) {
            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    if (bracket[1] == ']' || bracket[1] == '\0') {
                        fprintf(temp_file, "deps: [%s]\n", dep_id);
                    } else {
                        fprintf(temp_file, "%s, %s]\n", line, dep_id);
                    }
                    continue;
                }
            }
        }
        fprintf(temp_file, "%s", line);
    }
    
    fclose(file);
    fclose(temp_file);
    
    if (rename(temp_path, resolved_path) != 0) {
        fprintf(stderr, "Error: cannot update ticket file\n");
        unlink(temp_path);
        return 1;
    }
    
    return 0;
}

static int parse_links(const char *file_path, char links[MAX_LINKS][MAX_PATH], int *link_count) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        return 1;
    }
    
    *link_count = 0;
    char line[1024];
    int in_frontmatter = 0;
    
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            in_frontmatter = !in_frontmatter;
            continue;
        }
        
        if (in_frontmatter && strncmp(line, "links:", 6) == 0) {
            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    bracket++;
                    if (*bracket != '\0' && *bracket != ']') {
                        char *token = strtok(bracket, ",");
                        while (token != NULL && *link_count < MAX_LINKS) {
                            while (*token == ' ') token++;
                            char *token_end = token + strlen(token) - 1;
                            while (token_end > token && (*token_end == ' ' || *token_end == '\n')) {
                                *token_end = '\0';
                                token_end--;
                            }
                            if (*token != '\0') {
                                strncpy(links[*link_count], token, MAX_PATH - 1);
                                links[*link_count][MAX_PATH - 1] = '\0';
                                (*link_count)++;
                            }
                            token = strtok(NULL, ",");
                        }
                    }
                }
            }
            break;
        }
    }
    
    fclose(file);
    return 0;
}

static int has_link(char links[MAX_LINKS][MAX_PATH], int link_count, const char *link_id) {
    for (int i = 0; i < link_count; i++) {
        if (strcmp(links[i], link_id) == 0) {
            return 1;
        }
    }
    return 0;
}

static int add_link_to_file(const char *file_path, const char *link_id) {
    char existing_links[MAX_LINKS][MAX_PATH];
    int link_count = 0;
    
    if (parse_links(file_path, existing_links, &link_count) != 0) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    if (has_link(existing_links, link_count, link_id)) {
        return 0;
    }
    
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char temp_path[MAX_PATH];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", file_path);
    FILE *temp_file = fopen(temp_path, "w");
    if (temp_file == NULL) {
        fclose(file);
        fprintf(stderr, "Error: cannot write temp file\n");
        return 1;
    }
    
    char line[1024];
    int in_frontmatter = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            fprintf(temp_file, "%s", line);
            in_frontmatter = !in_frontmatter;
            continue;
        }
        
        if (in_frontmatter && strncmp(line, "links:", 6) == 0) {
            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    if (bracket[1] == ']' || bracket[1] == '\0') {
                        fprintf(temp_file, "links: [%s]\n", link_id);
                    } else {
                        fprintf(temp_file, "%s, %s]\n", line, link_id);
                    }
                    continue;
                }
            }
        }
        fprintf(temp_file, "%s", line);
    }
    
    fclose(file);
    fclose(temp_file);
    
    if (rename(temp_path, file_path) != 0) {
        fprintf(stderr, "Error: cannot update ticket file\n");
        unlink(temp_path);
        return 1;
    }
    
    return 0;
}

static int remove_link_from_file(const char *file_path, const char *link_id) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char temp_path[MAX_PATH];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", file_path);
    FILE *temp_file = fopen(temp_path, "w");
    if (temp_file == NULL) {
        fclose(file);
        fprintf(stderr, "Error: cannot write temp file\n");
        return 1;
    }
    
    char line[1024];
    int in_frontmatter = 0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            fprintf(temp_file, "%s", line);
            in_frontmatter = !in_frontmatter;
            continue;
        }
        
        if (in_frontmatter && strncmp(line, "links:", 6) == 0) {
            char existing_links[MAX_LINKS][MAX_PATH];
            int link_count = 0;
            
            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    bracket++;
                    if (*bracket != '\0' && *bracket != ']') {
                        char *token = strtok(bracket, ",");
                        while (token != NULL && link_count < MAX_LINKS) {
                            while (*token == ' ') token++;
                            char *token_end = token + strlen(token) - 1;
                            while (token_end > token && (*token_end == ' ' || *token_end == '\n')) {
                                *token_end = '\0';
                                token_end--;
                            }
                            if (*token != '\0' && strcmp(token, link_id) != 0) {
                                strncpy(existing_links[link_count], token, MAX_PATH - 1);
                                existing_links[link_count][MAX_PATH - 1] = '\0';
                                link_count++;
                            }
                            token = strtok(NULL, ",");
                        }
                    }
                }
            }
            
            fprintf(temp_file, "links: [");
            for (int i = 0; i < link_count; i++) {
                if (i > 0) fprintf(temp_file, ", ");
                fprintf(temp_file, "%s", existing_links[i]);
            }
            fprintf(temp_file, "]\n");
            continue;
        }
        fprintf(temp_file, "%s", line);
    }
    
    fclose(file);
    fclose(temp_file);
    
    if (rename(temp_path, file_path) != 0) {
        fprintf(stderr, "Error: cannot update ticket file\n");
        unlink(temp_path);
        return 1;
    }
    
    return 0;
}

static int cmd_link(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: at least two ticket IDs required\n");
        return 1;
    }
    
    int num_tickets = argc - 1;
    char resolved_paths[MAX_TICKETS][MAX_PATH];
    char ticket_ids[MAX_TICKETS][MAX_PATH];
    
    for (int i = 0; i < num_tickets; i++) {
        if (resolve_ticket_id(argv[i + 1], resolved_paths[i], sizeof(resolved_paths[i])) != 0) {
            return 1;
        }
        
        const char *basename = strrchr(resolved_paths[i], '/');
        basename = basename ? basename + 1 : resolved_paths[i];
        snprintf(ticket_ids[i], sizeof(ticket_ids[i]), "%.*s", (int)(strlen(basename) - 3), basename);
    }
    
    int total_added = 0;
    
    for (int i = 0; i < num_tickets; i++) {
        for (int j = 0; j < num_tickets; j++) {
            if (i != j) {
                char existing_links[MAX_LINKS][MAX_PATH];
                int link_count = 0;
                
                if (parse_links(resolved_paths[i], existing_links, &link_count) != 0) {
                    return 1;
                }
                
                if (!has_link(existing_links, link_count, ticket_ids[j])) {
                    if (add_link_to_file(resolved_paths[i], ticket_ids[j]) != 0) {
                        return 1;
                    }
                    total_added++;
                }
            }
        }
    }
    
    if (total_added == 0) {
        printf("All links already exist\n");
    } else {
        printf("Added %d link(s) between %d tickets\n", total_added, num_tickets);
    }
    
    return 0;
}

static int cmd_unlink(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: exactly two ticket IDs required\n");
        return 1;
    }
    
    char resolved_path1[MAX_PATH];
    if (resolve_ticket_id(argv[1], resolved_path1, sizeof(resolved_path1)) != 0) {
        return 1;
    }
    
    char resolved_path2[MAX_PATH];
    if (resolve_ticket_id(argv[2], resolved_path2, sizeof(resolved_path2)) != 0) {
        return 1;
    }
    
    char id1[MAX_PATH], id2[MAX_PATH];
    const char *basename1 = strrchr(resolved_path1, '/');
    basename1 = basename1 ? basename1 + 1 : resolved_path1;
    snprintf(id1, sizeof(id1), "%.*s", (int)(strlen(basename1) - 3), basename1);
    
    const char *basename2 = strrchr(resolved_path2, '/');
    basename2 = basename2 ? basename2 + 1 : resolved_path2;
    snprintf(id2, sizeof(id2), "%.*s", (int)(strlen(basename2) - 3), basename2);
    
    char existing_links[MAX_LINKS][MAX_PATH];
    int link_count = 0;
    
    if (parse_links(resolved_path1, existing_links, &link_count) != 0) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    if (!has_link(existing_links, link_count, id2)) {
        printf("Link not found\n");
        return 1;
    }
    
    if (remove_link_from_file(resolved_path1, id2) != 0) {
        return 1;
    }
    
    if (remove_link_from_file(resolved_path2, id1) != 0) {
        return 1;
    }
    
    printf("Removed link: %s <-> %s\n", id1, id2);
    return 0;
}

static int cmd_edit(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("edit");
}

static int cmd_add_note(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ticket add-note <id> [note text]\n");
        return 1;
    }
    
    const char *ticket_id = argv[1];
    char resolved_path[MAX_PATH];
    
    if (resolve_ticket_id(ticket_id, resolved_path, sizeof(resolved_path)) != 0) {
        return 1;
    }
    
    const char *basename = strrchr(resolved_path, '/');
    basename = basename ? basename + 1 : resolved_path;
    char target_id[MAX_PATH];
    snprintf(target_id, sizeof(target_id), "%.*s", (int)(strlen(basename) - 3), basename);
    
    char note[4096] = "";
    
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            if (i > 2) {
                strncat(note, " ", sizeof(note) - strlen(note) - 1);
            }
            strncat(note, argv[i], sizeof(note) - strlen(note) - 1);
        }
    } else if (!isatty(STDIN_FILENO)) {
        size_t total = 0;
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), stdin) != NULL && total < sizeof(note) - 1) {
            size_t len = strlen(buffer);
            if (total + len < sizeof(note)) {
                strcat(note, buffer);
                total += len;
            }
        }
        if (total > 0 && note[total - 1] == '\n') {
            note[total - 1] = '\0';
        }
    } else {
        fprintf(stderr, "Error: no note provided\n");
        return 1;
    }
    
    char timestamp[64];
    get_iso_date(timestamp, sizeof(timestamp));
    
    FILE *file = fopen(resolved_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }
    
    char content[65536];
    size_t content_len = 0;
    size_t bytes_read;
    while ((bytes_read = fread(content + content_len, 1, sizeof(content) - content_len - 1, file)) > 0) {
        content_len += bytes_read;
    }
    content[content_len] = '\0';
    fclose(file);
    
    if (strstr(content, "## Notes") == NULL) {
        strncat(content, "\n## Notes\n", sizeof(content) - strlen(content) - 1);
    }
    
    char note_entry[4096];
    snprintf(note_entry, sizeof(note_entry), "\n**%s**\n\n%s\n", timestamp, note);
    strncat(content, note_entry, sizeof(content) - strlen(content) - 1);
    
    file = fopen(resolved_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot write ticket file\n");
        return 1;
    }
    
    fputs(content, file);
    fclose(file);
    
    printf("Note added to %s\n", target_id);
    return 0;
}

static int cmd_query(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    return cmd_not_implemented("query");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];
    
    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 || strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 || strcmp(command, "-v") == 0) {
        print_version();
        return 0;
    }
    
    if (strcmp(command, "create") == 0) {
        return cmd_create(argc - 1, &argv[1]);
    } else if (strcmp(command, "show") == 0) {
        return cmd_show(argc - 1, &argv[1]);
    } else if (strcmp(command, "list") == 0) {
        return cmd_list(argc - 1, &argv[1]);
    } else if (strcmp(command, "status") == 0) {
        return cmd_status(argc - 1, &argv[1]);
    } else if (strcmp(command, "start") == 0) {
        return cmd_start(argc - 1, &argv[1]);
    } else if (strcmp(command, "close") == 0) {
        return cmd_close(argc - 1, &argv[1]);
    } else if (strcmp(command, "reopen") == 0) {
        return cmd_reopen(argc - 1, &argv[1]);
    } else if (strcmp(command, "dep") == 0) {
        return cmd_dep(argc - 1, &argv[1]);
    } else if (strcmp(command, "link") == 0) {
        return cmd_link(argc - 1, &argv[1]);
    } else if (strcmp(command, "unlink") == 0) {
        return cmd_unlink(argc - 1, &argv[1]);
    } else if (strcmp(command, "edit") == 0) {
        return cmd_edit(argc - 1, &argv[1]);
    } else if (strcmp(command, "add-note") == 0) {
        return cmd_add_note(argc - 1, &argv[1]);
    } else if (strcmp(command, "query") == 0) {
        return cmd_query(argc - 1, &argv[1]);
    } else {
        fprintf(stderr, "Error: unknown command '%s'\n", command);
        fprintf(stderr, "Run '%s help' for usage information\n", argv[0]);
        return 1;
    }
    
    return 0;
}
