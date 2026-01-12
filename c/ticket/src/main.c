#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

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

static int cmd_create(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    return cmd_not_implemented("create");
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
    if (argc < 3) {
        fprintf(stderr, "Error: ticket ID and note text required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("add-note");
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
