#include <dirent.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define TICKETS_DIR ".tickets"
#define MAX_PATH 1024
#define MAX_MATCHES 100
#define MAX_LINKS 100
#define MAX_DEPS 100
#define MAX_TICKETS 1000

static void print_usage(const char *program_name)
{
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
    printf("  undep <id> <dep-id>         Remove dependency\n");
    printf("  dep tree [--full] <id>      Show dependency tree\n");
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

static void print_version(void)
{
    printf("ticket-cli (C implementation) version %s\n", VERSION);
}

static int resolve_ticket_id(const char *ticket_id, char *resolved_path, size_t path_size)
{
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

static int cmd_not_implemented(const char *command)
{
    fprintf(stderr, "Command '%s' not yet implemented\n", command);
    return 1;
}

static void get_iso_date(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *utc = gmtime(&now);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", utc);
}

static void generate_ticket_id(char *buffer, size_t size)
{
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
    SHA256((unsigned char *)entropy, strlen(entropy), hash);

    snprintf(buffer, size, "%s-%02x%02x", prefix, hash[0], hash[1]);
}

static void ensure_tickets_dir(void)
{
    struct stat st = {0};
    if (stat(TICKETS_DIR, &st) == -1) {
        mkdir(TICKETS_DIR, 0755);
    }
}

static int cmd_create(int argc, char *argv[])
{
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
            if (len > 0 && assignee[len - 1] == '\n') {
                assignee[len - 1] = '\0';
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

static int cmd_show(int argc, char *argv[])
{
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

static int cmd_status(int argc, char *argv[])
{
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

static int cmd_start(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("start");
}

static int cmd_close(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("close");
}

static int cmd_reopen(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("reopen");
}

static int parse_deps(const char *file_path, char deps[MAX_DEPS][MAX_PATH], int *dep_count);
static int has_dep(char deps[MAX_DEPS][MAX_PATH], int dep_count, const char *dep_id);
static int add_dep_to_file(const char *file_path, const char *dep_id);
static int cmd_dep_tree(int argc, char *argv[]);
static int cmd_undep(int argc, char *argv[]);

static int cmd_dep(int argc, char *argv[])
{
    if (argc < 1) {
        fprintf(stderr, "Usage: ticket dep <id> <dependency-id>\n");
        fprintf(stderr, "       ticket dep tree [--full] <id>  - show dependency tree\n");
        return 1;
    }

    if (strcmp(argv[1], "tree") == 0) {
        return cmd_dep_tree(argc - 1, &argv[1]);
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: ticket dep <id> <dependency-id>\n");
        fprintf(stderr, "       ticket dep tree [--full] <id>  - show dependency tree\n");
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

    const char *basename1 = strrchr(resolved_path, '/');
    basename1 = basename1 ? basename1 + 1 : resolved_path;
    char ticket_id[MAX_PATH];
    snprintf(ticket_id, sizeof(ticket_id), "%.*s", (int)(strlen(basename1) - 3), basename1);

    const char *basename2 = strrchr(dep_path, '/');
    basename2 = basename2 ? basename2 + 1 : dep_path;
    char dep_id[MAX_PATH];
    snprintf(dep_id, sizeof(dep_id), "%.*s", (int)(strlen(basename2) - 3), basename2);

    char existing_deps[MAX_DEPS][MAX_PATH];
    int dep_count = 0;

    if (parse_deps(resolved_path, existing_deps, &dep_count) != 0) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }

    if (has_dep(existing_deps, dep_count, dep_id)) {
        printf("Dependency already exists\n");
        return 0;
    }

    if (add_dep_to_file(resolved_path, dep_id) != 0) {
        return 1;
    }

    printf("Added dependency: %s -> %s\n", ticket_id, dep_id);
    return 0;
}

static int parse_links(const char *file_path, char links[MAX_LINKS][MAX_PATH], int *link_count)
{
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
                            while (*token == ' ')
                                token++;
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

static int has_link(char links[MAX_LINKS][MAX_PATH], int link_count, const char *link_id)
{
    for (int i = 0; i < link_count; i++) {
        if (strcmp(links[i], link_id) == 0) {
            return 1;
        }
    }
    return 0;
}

static int add_link_to_file(const char *file_path, const char *link_id)
{
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

static int remove_link_from_file(const char *file_path, const char *link_id)
{
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
                            while (*token == ' ')
                                token++;
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
                if (i > 0)
                    fprintf(temp_file, ", ");
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

static int parse_deps(const char *file_path, char deps[MAX_DEPS][MAX_PATH], int *dep_count)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        return 1;
    }

    *dep_count = 0;
    char line[1024];
    int in_frontmatter = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strcmp(line, "---\n") == 0) {
            in_frontmatter = !in_frontmatter;
            continue;
        }

        if (in_frontmatter && strncmp(line, "deps:", 5) == 0) {
            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    bracket++;
                    if (*bracket != '\0' && *bracket != ']') {
                        char *token = strtok(bracket, ",");
                        while (token != NULL && *dep_count < MAX_DEPS) {
                            while (*token == ' ')
                                token++;
                            char *token_end = token + strlen(token) - 1;
                            while (token_end > token && (*token_end == ' ' || *token_end == '\n')) {
                                *token_end = '\0';
                                token_end--;
                            }
                            if (*token != '\0') {
                                strncpy(deps[*dep_count], token, MAX_PATH - 1);
                                deps[*dep_count][MAX_PATH - 1] = '\0';
                                (*dep_count)++;
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

static int has_dep(char deps[MAX_DEPS][MAX_PATH], int dep_count, const char *dep_id)
{
    for (int i = 0; i < dep_count; i++) {
        if (strcmp(deps[i], dep_id) == 0) {
            return 1;
        }
    }
    return 0;
}

static int add_dep_to_file(const char *file_path, const char *dep_id)
{
    char existing_deps[MAX_DEPS][MAX_PATH];
    int dep_count = 0;

    if (parse_deps(file_path, existing_deps, &dep_count) != 0) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }

    if (has_dep(existing_deps, dep_count, dep_id)) {
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

    if (rename(temp_path, file_path) != 0) {
        fprintf(stderr, "Error: cannot update ticket file\n");
        unlink(temp_path);
        return 1;
    }

    return 0;
}

static int remove_dep_from_file(const char *file_path, const char *dep_id)
{
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

        if (in_frontmatter && strncmp(line, "deps:", 5) == 0) {
            char existing_deps[MAX_DEPS][MAX_PATH];
            int dep_count = 0;

            char *bracket = strchr(line, '[');
            if (bracket != NULL) {
                char *end = strchr(bracket, ']');
                if (end != NULL) {
                    *end = '\0';
                    bracket++;
                    if (*bracket != '\0' && *bracket != ']') {
                        char *token = strtok(bracket, ",");
                        while (token != NULL && dep_count < MAX_DEPS) {
                            while (*token == ' ')
                                token++;
                            char *token_end = token + strlen(token) - 1;
                            while (token_end > token && (*token_end == ' ' || *token_end == '\n')) {
                                *token_end = '\0';
                                token_end--;
                            }
                            if (*token != '\0' && strcmp(token, dep_id) != 0) {
                                strncpy(existing_deps[dep_count], token, MAX_PATH - 1);
                                existing_deps[dep_count][MAX_PATH - 1] = '\0';
                                dep_count++;
                            }
                            token = strtok(NULL, ",");
                        }
                    }
                }
            }

            fprintf(temp_file, "deps: [");
            for (int i = 0; i < dep_count; i++) {
                if (i > 0)
                    fprintf(temp_file, ", ");
                fprintf(temp_file, "%s", existing_deps[i]);
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

static int cmd_link(int argc, char *argv[])
{
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
        snprintf(ticket_ids[i], sizeof(ticket_ids[i]), "%.*s", (int)(strlen(basename) - 3),
                 basename);
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

static int cmd_unlink(int argc, char *argv[])
{
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

static int cmd_edit(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Error: ticket ID required\n");
        return 1;
    }
    (void)argv;
    return cmd_not_implemented("edit");
}

static int cmd_add_note(int argc, char *argv[])
{
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
    while ((bytes_read = fread(content + content_len, 1, sizeof(content) - content_len - 1, file)) >
           0) {
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

static int cmd_undep(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: ticket undep <id> <dependency-id>\n");
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

    const char *basename1 = strrchr(resolved_path, '/');
    basename1 = basename1 ? basename1 + 1 : resolved_path;
    char ticket_id[MAX_PATH];
    snprintf(ticket_id, sizeof(ticket_id), "%.*s", (int)(strlen(basename1) - 3), basename1);

    const char *basename2 = strrchr(dep_path, '/');
    basename2 = basename2 ? basename2 + 1 : dep_path;
    char dep_id[MAX_PATH];
    snprintf(dep_id, sizeof(dep_id), "%.*s", (int)(strlen(basename2) - 3), basename2);

    char existing_deps[MAX_DEPS][MAX_PATH];
    int dep_count = 0;

    if (parse_deps(resolved_path, existing_deps, &dep_count) != 0) {
        fprintf(stderr, "Error: cannot read ticket file\n");
        return 1;
    }

    if (!has_dep(existing_deps, dep_count, dep_id)) {
        printf("Dependency not found\n");
        return 1;
    }

    if (remove_dep_from_file(resolved_path, dep_id) != 0) {
        return 1;
    }

    printf("Removed dependency: %s -/-> %s\n", ticket_id, dep_id);
    return 0;
}

typedef struct {
    char id[MAX_PATH];
    char status[64];
    char title[256];
    char deps[MAX_DEPS][MAX_PATH];
    int dep_count;
    int priority;
    int subtree_depth;
    int visited;
} Ticket;

static int load_all_tickets(Ticket *tickets, int *ticket_count)
{
    DIR *dir = opendir(TICKETS_DIR);
    if (dir == NULL) {
        return 1;
    }

    *ticket_count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && *ticket_count < MAX_TICKETS) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 3, ".md") != 0) {
            continue;
        }

        char file_path[MAX_PATH];
        snprintf(file_path, sizeof(file_path), "%s/%s", TICKETS_DIR, entry->d_name);

        FILE *file = fopen(file_path, "r");
        if (file == NULL) {
            continue;
        }

        Ticket *t = &tickets[*ticket_count];
        snprintf(t->id, sizeof(t->id), "%.*s", (int)(len - 3), entry->d_name);
        strcpy(t->status, "open");
        strcpy(t->title, "");
        t->dep_count = 0;
        t->priority = 2;
        t->subtree_depth = 0;
        t->visited = 0;

        char line[1024];
        int in_frontmatter = 0;
        int got_title = 0;

        while (fgets(line, sizeof(line), file) != NULL) {
            if (strcmp(line, "---\n") == 0) {
                in_frontmatter = !in_frontmatter;
                continue;
            }

            if (in_frontmatter) {
                if (strncmp(line, "status:", 7) == 0) {
                    sscanf(line, "status: %63s", t->status);
                } else if (strncmp(line, "priority:", 9) == 0) {
                    sscanf(line, "priority: %d", &t->priority);
                } else if (strncmp(line, "deps:", 5) == 0) {
                    char *bracket = strchr(line, '[');
                    if (bracket != NULL) {
                        char *end = strchr(bracket, ']');
                        if (end != NULL) {
                            *end = '\0';
                            bracket++;
                            if (*bracket != '\0' && *bracket != ']') {
                                char *token = strtok(bracket, ",");
                                while (token != NULL && t->dep_count < MAX_DEPS) {
                                    while (*token == ' ')
                                        token++;
                                    char *token_end = token + strlen(token) - 1;
                                    while (token_end > token && *token_end == ' ') {
                                        *token_end = '\0';
                                        token_end--;
                                    }
                                    if (*token != '\0') {
                                        strncpy(t->deps[t->dep_count], token, MAX_PATH - 1);
                                        t->deps[t->dep_count][MAX_PATH - 1] = '\0';
                                        t->dep_count++;
                                    }
                                    token = strtok(NULL, ",");
                                }
                            }
                        }
                    }
                }
            } else if (!got_title && strncmp(line, "# ", 2) == 0) {
                size_t title_len = strlen(line + 2);
                if (title_len > 0 && line[2 + title_len - 1] == '\n') {
                    title_len--;
                }
                snprintf(t->title, sizeof(t->title), "%.*s", (int)title_len, line + 2);
                got_title = 1;
            }
        }

        fclose(file);
        (*ticket_count)++;
    }

    closedir(dir);
    return 0;
}

static int find_ticket(Ticket *tickets, int ticket_count, const char *id)
{
    for (int i = 0; i < ticket_count; i++) {
        if (strcmp(tickets[i].id, id) == 0) {
            return i;
        }
    }
    return -1;
}

static int ticket_compare_by_priority_and_id(const void *a, const void *b)
{
    const Ticket *t1 = (const Ticket *)a;
    const Ticket *t2 = (const Ticket *)b;

    if (t1->priority != t2->priority) {
        return t1->priority - t2->priority;
    }

    return strcmp(t1->id, t2->id);
}

static int ticket_is_ready(const Ticket *ticket, Ticket *all_tickets, int ticket_count)
{
    if (strcmp(ticket->status, "open") != 0 && strcmp(ticket->status, "in_progress") != 0) {
        return 0;
    }

    for (int i = 0; i < ticket->dep_count; i++) {
        int dep_idx = find_ticket(all_tickets, ticket_count, ticket->deps[i]);
        if (dep_idx < 0) {
            return 0;
        }
        if (strcmp(all_tickets[dep_idx].status, "closed") != 0) {
            return 0;
        }
    }

    return 1;
}

static int ticket_is_blocked(const Ticket *ticket, Ticket *all_tickets, int ticket_count)
{
    if (strcmp(ticket->status, "open") != 0 && strcmp(ticket->status, "in_progress") != 0) {
        return 0;
    }

    if (ticket->dep_count == 0) {
        return 0;
    }

    for (int i = 0; i < ticket->dep_count; i++) {
        int dep_idx = find_ticket(all_tickets, ticket_count, ticket->deps[i]);
        if (dep_idx < 0) {
            return 1;
        }
        if (strcmp(all_tickets[dep_idx].status, "closed") != 0) {
            return 1;
        }
    }

    return 0;
}

static int cmd_ls(int argc, char *argv[])
{
    Ticket tickets[MAX_TICKETS];
    int ticket_count = 0;

    if (load_all_tickets(tickets, &ticket_count) != 0) {
        return 0;
    }

    char status_filter[64] = "";
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--status=", 9) == 0) {
            strncpy(status_filter, argv[i] + 9, sizeof(status_filter) - 1);
            status_filter[sizeof(status_filter) - 1] = '\0';
        }
    }

    qsort(tickets, ticket_count, sizeof(Ticket), ticket_compare_by_priority_and_id);

    for (int i = 0; i < ticket_count; i++) {
        Ticket *t = &tickets[i];

        if (status_filter[0] != '\0' && strcmp(t->status, status_filter) != 0) {
            continue;
        }

        printf("%-8s [%s] - %s", t->id, t->status, t->title);

        if (t->dep_count > 0) {
            printf(" <- [");
            for (int j = 0; j < t->dep_count; j++) {
                if (j > 0)
                    printf(", ");
                printf("%s", t->deps[j]);
            }
            printf("]");
        }

        printf("\n");
    }

    return 0;
}

static int cmd_list(int argc, char *argv[])
{
    return cmd_ls(argc, argv);
}

static int cmd_ready(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Ticket tickets[MAX_TICKETS];
    int ticket_count = 0;

    if (load_all_tickets(tickets, &ticket_count) != 0) {
        return 0;
    }

    Ticket ready_tickets[MAX_TICKETS];
    int ready_count = 0;

    for (int i = 0; i < ticket_count; i++) {
        if (ticket_is_ready(&tickets[i], tickets, ticket_count)) {
            ready_tickets[ready_count++] = tickets[i];
        }
    }

    qsort(ready_tickets, ready_count, sizeof(Ticket), ticket_compare_by_priority_and_id);

    for (int i = 0; i < ready_count; i++) {
        Ticket *t = &ready_tickets[i];
        printf("%-8s [P%d][%s] - %s\n", t->id, t->priority, t->status, t->title);
    }

    return 0;
}

static int cmd_blocked(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Ticket tickets[MAX_TICKETS];
    int ticket_count = 0;

    if (load_all_tickets(tickets, &ticket_count) != 0) {
        return 0;
    }

    Ticket blocked_tickets[MAX_TICKETS];
    int blocked_count = 0;

    for (int i = 0; i < ticket_count; i++) {
        if (ticket_is_blocked(&tickets[i], tickets, ticket_count)) {
            blocked_tickets[blocked_count++] = tickets[i];
        }
    }

    qsort(blocked_tickets, blocked_count, sizeof(Ticket), ticket_compare_by_priority_and_id);

    for (int i = 0; i < blocked_count; i++) {
        Ticket *t = &blocked_tickets[i];
        printf("%-8s [P%d][%s] - %s", t->id, t->priority, t->status, t->title);

        int first = 1;
        printf(" <- [");
        for (int j = 0; j < t->dep_count; j++) {
            int dep_idx = find_ticket(tickets, ticket_count, t->deps[j]);
            if (dep_idx < 0 || strcmp(tickets[dep_idx].status, "closed") != 0) {
                if (!first)
                    printf(", ");
                printf("%s", t->deps[j]);
                first = 0;
            }
        }
        printf("]\n");
    }

    return 0;
}

static int cmd_closed(int argc, char *argv[])
{
    int limit = 20;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--limit=", 8) == 0) {
            limit = atoi(argv[i] + 8);
        }
    }

    DIR *dir = opendir(TICKETS_DIR);
    if (dir == NULL) {
        return 0;
    }

    typedef struct {
        char path[MAX_PATH];
        time_t mtime;
    } FileInfo;

    FileInfo files[MAX_TICKETS];
    int file_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_TICKETS) {
        if (entry->d_name[0] == '.')
            continue;

        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 3, ".md") != 0)
            continue;

        char file_path[MAX_PATH];
        snprintf(file_path, sizeof(file_path), "%s/%s", TICKETS_DIR, entry->d_name);

        struct stat st;
        if (stat(file_path, &st) == 0) {
            strncpy(files[file_count].path, file_path, MAX_PATH - 1);
            files[file_count].path[MAX_PATH - 1] = '\0';
            files[file_count].mtime = st.st_mtime;
            file_count++;
        }
    }

    closedir(dir);

    for (int i = 0; i < file_count - 1; i++) {
        for (int j = i + 1; j < file_count; j++) {
            if (files[i].mtime < files[j].mtime) {
                FileInfo temp = files[i];
                files[i] = files[j];
                files[j] = temp;
            }
        }
    }

    int closed_count = 0;
    int max_check = file_count < 100 ? file_count : 100;

    for (int i = 0; i < max_check && closed_count < limit; i++) {
        FILE *file = fopen(files[i].path, "r");
        if (file == NULL)
            continue;

        char ticket_id[MAX_PATH] = "";
        char status[64] = "open";
        char title[256] = "";

        const char *basename = strrchr(files[i].path, '/');
        basename = basename ? basename + 1 : files[i].path;
        size_t basename_len = strlen(basename);
        snprintf(ticket_id, sizeof(ticket_id), "%.*s", (int)(basename_len - 3), basename);

        char line[1024];
        int in_frontmatter = 0;
        int got_title = 0;

        while (fgets(line, sizeof(line), file) != NULL) {
            if (strcmp(line, "---\n") == 0) {
                in_frontmatter = !in_frontmatter;
                continue;
            }

            if (in_frontmatter && strncmp(line, "status:", 7) == 0) {
                sscanf(line, "status: %63s", status);
            } else if (!got_title && strncmp(line, "# ", 2) == 0) {
                size_t title_len = strlen(line + 2);
                if (title_len > 0 && line[2 + title_len - 1] == '\n') {
                    title_len--;
                }
                snprintf(title, sizeof(title), "%.*s", (int)title_len, line + 2);
                got_title = 1;
            }
        }

        fclose(file);

        if (strcmp(status, "closed") == 0 || strcmp(status, "done") == 0) {
            printf("%-8s [%s] - %s\n", ticket_id, status, title);
            closed_count++;
        }
    }

    return 0;
}

static int compute_subtree_depth(Ticket *tickets, int ticket_count, int idx, int path_mask[],
                                 int path_len)
{
    if (idx < 0 || idx >= ticket_count) {
        return 0;
    }

    for (int i = 0; i < path_len; i++) {
        if (path_mask[i] == idx) {
            return 0;
        }
    }

    if (tickets[idx].visited) {
        return tickets[idx].subtree_depth;
    }

    int max_depth = 0;
    path_mask[path_len] = idx;

    for (int i = 0; i < tickets[idx].dep_count; i++) {
        int dep_idx = find_ticket(tickets, ticket_count, tickets[idx].deps[i]);
        if (dep_idx >= 0) {
            int depth =
                compute_subtree_depth(tickets, ticket_count, dep_idx, path_mask, path_len + 1);
            if (depth + 1 > max_depth) {
                max_depth = depth + 1;
            }
        }
    }

    tickets[idx].subtree_depth = max_depth;
    tickets[idx].visited = 1;
    return max_depth;
}

static void print_dep_tree_recursive(Ticket *tickets, int ticket_count, int idx, const char *prefix,
                                     int is_last, int visited[], int visited_count, int full_mode)
{
    if (idx < 0)
        return;

    for (int i = 0; i < visited_count; i++) {
        if (visited[i] == idx) {
            return;
        }
    }

    Ticket *t = &tickets[idx];

    if (strcmp(prefix, "") == 0) {
        printf("%s [%s] %s\n", t->id, t->status, t->title);
    } else {
        const char *connector = is_last ? "└── " : "├── ";
        printf("%s%s%s [%s] %s\n", prefix, connector, t->id, t->status, t->title);
    }

    visited[visited_count] = idx;
    visited_count++;

    if (t->dep_count == 0) {
        return;
    }

    int dep_indices[MAX_DEPS];
    int sorted_positions[MAX_DEPS];
    for (int i = 0; i < t->dep_count; i++) {
        dep_indices[i] = find_ticket(tickets, ticket_count, t->deps[i]);
        sorted_positions[i] = i;
    }

    for (int i = 0; i < t->dep_count - 1; i++) {
        for (int j = i + 1; j < t->dep_count; j++) {
            int idx_i = dep_indices[sorted_positions[i]];
            int idx_j = dep_indices[sorted_positions[j]];

            if (idx_i < 0 && idx_j >= 0) {
                int temp = sorted_positions[i];
                sorted_positions[i] = sorted_positions[j];
                sorted_positions[j] = temp;
            } else if (idx_i >= 0 && idx_j >= 0) {
                Ticket *t_i = &tickets[idx_i];
                Ticket *t_j = &tickets[idx_j];

                int should_swap = 0;
                if (t_i->subtree_depth != t_j->subtree_depth) {
                    should_swap = t_i->subtree_depth < t_j->subtree_depth;
                } else {
                    should_swap = strcmp(t_i->id, t_j->id) > 0;
                }

                if (should_swap) {
                    int temp = sorted_positions[i];
                    sorted_positions[i] = sorted_positions[j];
                    sorted_positions[j] = temp;
                }
            }
        }
    }

    char new_prefix[MAX_PATH * 2];
    for (int i = 0; i < t->dep_count; i++) {
        int dep_pos = sorted_positions[i];
        int dep_idx = dep_indices[dep_pos];

        if (dep_idx < 0)
            continue;

        int is_last_child = (i == t->dep_count - 1);
        if (!full_mode) {
            is_last_child = 1;
            for (int j = i + 1; j < t->dep_count; j++) {
                if (dep_indices[sorted_positions[j]] >= 0) {
                    is_last_child = 0;
                    break;
                }
            }
        }

        if (strcmp(prefix, "") == 0) {
            strcpy(new_prefix, "");
        } else if (is_last) {
            snprintf(new_prefix, sizeof(new_prefix), "%s    ", prefix);
        } else {
            snprintf(new_prefix, sizeof(new_prefix), "%s│   ", prefix);
        }

        print_dep_tree_recursive(tickets, ticket_count, dep_idx, new_prefix, is_last_child, visited,
                                 visited_count, full_mode);
    }
}

static int cmd_dep_tree(int argc, char *argv[])
{
    int full_mode = 0;
    const char *root_id = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--full") == 0) {
            full_mode = 1;
        } else {
            root_id = argv[i];
        }
    }

    if (root_id == NULL) {
        fprintf(stderr, "Usage: ticket dep tree [--full] <id>\n");
        return 1;
    }

    Ticket tickets[MAX_TICKETS];
    int ticket_count = 0;

    if (load_all_tickets(tickets, &ticket_count) != 0) {
        fprintf(stderr, "Error: cannot load tickets\n");
        return 1;
    }

    int root_idx = -1;
    int match_count = 0;
    for (int i = 0; i < ticket_count; i++) {
        if (strstr(tickets[i].id, root_id) != NULL) {
            root_idx = i;
            match_count++;
        }
    }

    if (match_count == 0) {
        fprintf(stderr, "Error: ticket '%s' not found\n", root_id);
        return 1;
    }

    if (match_count > 1) {
        fprintf(stderr, "Error: ambiguous ID '%s' matches multiple tickets\n", root_id);
        return 1;
    }

    int path_mask[MAX_TICKETS];
    for (int i = 0; i < ticket_count; i++) {
        tickets[i].visited = 0;
    }
    for (int i = 0; i < ticket_count; i++) {
        compute_subtree_depth(tickets, ticket_count, i, path_mask, 0);
    }

    int visited[MAX_TICKETS];
    print_dep_tree_recursive(tickets, ticket_count, root_idx, "", 1, visited, 0, full_mode);

    return 0;
}

static void escape_json_string(const char *str, char *out, size_t out_size)
{
    size_t j = 0;
    for (size_t i = 0; str[i] != '\0' && j < out_size - 2; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            out[j++] = '\\';
            if (j >= out_size - 2)
                break;
            out[j++] = str[i];
        } else if (str[i] == '\n') {
            out[j++] = '\\';
            if (j >= out_size - 2)
                break;
            out[j++] = 'n';
        } else if (str[i] == '\r') {
            out[j++] = '\\';
            if (j >= out_size - 2)
                break;
            out[j++] = 'r';
        } else if (str[i] == '\t') {
            out[j++] = '\\';
            if (j >= out_size - 2)
                break;
            out[j++] = 't';
        } else {
            out[j++] = str[i];
        }
    }
    out[j] = '\0';
}

static void parse_array_field(const char *value, char items[MAX_DEPS][MAX_PATH], int *count)
{
    *count = 0;
    const char *bracket = strchr(value, '[');
    if (bracket == NULL)
        return;

    char *end = strchr(bracket, ']');
    if (end == NULL)
        return;

    char buffer[4096];
    size_t len = end - bracket - 1;
    if (len >= sizeof(buffer))
        len = sizeof(buffer) - 1;
    strncpy(buffer, bracket + 1, len);
    buffer[len] = '\0';

    if (strlen(buffer) == 0)
        return;

    char *token = strtok(buffer, ",");
    while (token != NULL && *count < MAX_DEPS) {
        while (*token == ' ')
            token++;
        char *token_end = token + strlen(token) - 1;
        while (token_end > token && (*token_end == ' ' || *token_end == '\n')) {
            *token_end = '\0';
            token_end--;
        }
        if (*token != '\0') {
            strncpy(items[*count], token, MAX_PATH - 1);
            items[*count][MAX_PATH - 1] = '\0';
            (*count)++;
        }
        token = strtok(NULL, ",");
    }
}

static int cmd_query(int argc, char *argv[])
{
    const char *jq_filter = (argc > 1) ? argv[1] : NULL;

    DIR *dir = opendir(TICKETS_DIR);
    if (dir == NULL) {
        return 0;
    }

    char **json_lines = malloc(MAX_TICKETS * sizeof(char *));
    if (json_lines == NULL) {
        closedir(dir);
        return 1;
    }
    int line_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && line_count < MAX_TICKETS) {
        if (entry->d_name[0] == '.')
            continue;

        size_t len = strlen(entry->d_name);
        if (len < 4 || strcmp(entry->d_name + len - 3, ".md") != 0)
            continue;

        char file_path[MAX_PATH];
        snprintf(file_path, sizeof(file_path), "%s/%s", TICKETS_DIR, entry->d_name);

        FILE *file = fopen(file_path, "r");
        if (file == NULL)
            continue;

        char json[8192] = "{";
        int in_frontmatter = 0;
        int frontmatter_started = 0;
        int first_field = 1;
        char line[1024];

        while (fgets(line, sizeof(line), file) != NULL) {
            if (strcmp(line, "---\n") == 0) {
                if (!frontmatter_started) {
                    in_frontmatter = 1;
                    frontmatter_started = 1;
                } else {
                    in_frontmatter = 0;
                    break;
                }
                continue;
            }

            if (in_frontmatter && strchr(line, ':') != NULL) {
                char key[256], value[4096];
                char *colon = strchr(line, ':');
                size_t key_len = colon - line;
                if (key_len >= sizeof(key))
                    key_len = sizeof(key) - 1;
                strncpy(key, line, key_len);
                key[key_len] = '\0';

                char *val_start = colon + 1;
                while (*val_start == ' ')
                    val_start++;
                strncpy(value, val_start, sizeof(value) - 1);
                value[sizeof(value) - 1] = '\0';

                size_t val_len = strlen(value);
                if (val_len > 0 && value[val_len - 1] == '\n') {
                    value[val_len - 1] = '\0';
                }

                if (!first_field) {
                    strcat(json, ",");
                }
                first_field = 0;

                char escaped_key[512];
                escape_json_string(key, escaped_key, sizeof(escaped_key));

                char field[4096];
                snprintf(field, sizeof(field), "\"%s\":", escaped_key);
                strcat(json, field);

                if (strcmp(key, "deps") == 0 || strcmp(key, "links") == 0) {
                    char items[MAX_DEPS][MAX_PATH];
                    int count;
                    parse_array_field(value, items, &count);

                    strcat(json, "[");
                    for (int i = 0; i < count; i++) {
                        if (i > 0)
                            strcat(json, ",");
                        char escaped_item[MAX_PATH * 2];
                        escape_json_string(items[i], escaped_item, sizeof(escaped_item));
                        char item_json[MAX_PATH * 2 + 3];
                        snprintf(item_json, sizeof(item_json), "\"%s\"", escaped_item);
                        strcat(json, item_json);
                    }
                    strcat(json, "]");
                } else if (strcmp(key, "priority") == 0) {
                    char num[64];
                    snprintf(num, sizeof(num), "%s", value);
                    strcat(json, num);
                } else {
                    char escaped_value[4096];
                    escape_json_string(value, escaped_value, sizeof(escaped_value));
                    char value_json[4096];
                    snprintf(value_json, sizeof(value_json), "\"%s\"", escaped_value);
                    strcat(json, value_json);
                }
            }
        }

        strcat(json, "}");
        fclose(file);

        json_lines[line_count] = strdup(json);
        line_count++;
    }

    closedir(dir);

    if (jq_filter) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            for (int i = 0; i < line_count; i++)
                free(json_lines[i]);
            free(json_lines);
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1) {
            close(pipefd[0]);
            close(pipefd[1]);
            for (int i = 0; i < line_count; i++)
                free(json_lines[i]);
            free(json_lines);
            return 1;
        }

        if (pid == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            char filter_arg[1024];
            snprintf(filter_arg, sizeof(filter_arg), "select(%s)", jq_filter);
            execlp("jq", "jq", "-c", filter_arg, NULL);
            exit(1);
        } else {
            close(pipefd[0]);

            for (int i = 0; i < line_count; i++) {
                write(pipefd[1], json_lines[i], strlen(json_lines[i]));
                write(pipefd[1], "\n", 1);
                free(json_lines[i]);
            }
            close(pipefd[1]);

            int status;
            waitpid(pid, &status, 0);
            free(json_lines);
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    } else {
        for (int i = 0; i < line_count; i++) {
            printf("%s\n", json_lines[i]);
            free(json_lines[i]);
        }
        free(json_lines);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "help") == 0 || strcmp(command, "--help") == 0 ||
        strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(command, "version") == 0 || strcmp(command, "--version") == 0 ||
        strcmp(command, "-v") == 0) {
        print_version();
        return 0;
    }

    if (strcmp(command, "create") == 0) {
        return cmd_create(argc - 1, &argv[1]);
    } else if (strcmp(command, "show") == 0) {
        return cmd_show(argc - 1, &argv[1]);
    } else if (strcmp(command, "list") == 0) {
        return cmd_list(argc - 1, &argv[1]);
    } else if (strcmp(command, "ls") == 0) {
        return cmd_ls(argc - 1, &argv[1]);
    } else if (strcmp(command, "ready") == 0) {
        return cmd_ready(argc - 1, &argv[1]);
    } else if (strcmp(command, "blocked") == 0) {
        return cmd_blocked(argc - 1, &argv[1]);
    } else if (strcmp(command, "closed") == 0) {
        return cmd_closed(argc - 1, &argv[1]);
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
    } else if (strcmp(command, "undep") == 0) {
        return cmd_undep(argc - 1, &argv[1]);
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
