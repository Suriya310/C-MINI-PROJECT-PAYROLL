/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║         EMPLOYEE PAYROLL MANAGEMENT SYSTEM  v2.0            ║
 * ║         Terminal Edition — Menu Driven, Binary I/O          ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 *  PAYROLL FORMULA
 *  ───────────────
 *  OT Pay    = OT Hours × Rs.100 / hour
 *  Gross Pay = Basic Pay + OT Pay
 *  Tax Slab  : Gross ≤ 30 000  →  5 %
 *              Gross ≤ 60 000  → 10 %
 *              Gross >  60 000  → 15 %
 *  Net Pay   = Gross Pay − Tax Amount
 *
 *  FILES
 *  ─────
 *  payroll_data.dat   (binary, auto-saved on exit)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ───────────────────────────────────────────────────────────── */
/*  CONSTANTS                                                    */
/* ───────────────────────────────────────────────────────────── */
#define MAX_EMPLOYEES  100
#define OT_RATE        100.0
#define DATA_FILE      "payroll_data.dat"
#define NAME_LEN       50
#define ID_LEN         15
#define DEPT_LEN       30

/* ANSI colour codes (disable if your terminal doesn't support) */
#define COL_RESET   "\033[0m"
#define COL_BOLD    "\033[1m"
#define COL_CYAN    "\033[96m"
#define COL_GREEN   "\033[92m"
#define COL_YELLOW  "\033[93m"
#define COL_RED     "\033[91m"
#define COL_MAGENTA "\033[95m"
#define COL_BLUE    "\033[94m"
#define COL_DIM     "\033[2m"

/* ───────────────────────────────────────────────────────────── */
/*  EMPLOYEE RECORD                                              */
/* ───────────────────────────────────────────────────────────── */
typedef struct {
    char   empId[ID_LEN];    /* Unique ID (normalised UPPERCASE) */
    char   name[NAME_LEN];   /* Full name                        */
    char   dept[DEPT_LEN];   /* Department                       */
    double basicPay;          /* Monthly basic salary (Rs.)       */
    double otHours;           /* Overtime hours this month        */
    /* ── Computed Fields (filled by calculatePayroll) ── */
    double otPay;
    double grossPay;
    double taxRate;
    double taxAmount;
    double netPay;
} Employee;

/* ───────────────────────────────────────────────────────────── */
/*  GLOBAL STATE                                                 */
/* ───────────────────────────────────────────────────────────── */
Employee employees[MAX_EMPLOYEES];
int      empCount = 0;

/* ───────────────────────────────────────────────────────────── */
/*  FUNCTION PROTOTYPES                                          */
/* ───────────────────────────────────────────────────────────── */
/* Core Operations */
void   addEmployee(void);
void   displayAll(void);
void   generatePayslip(void);
void   deleteEmployee(void);
void   searchEmployee(void);
void   editEmployee(void);

/* File I/O */
void   saveToFile(void);
void   loadFromFile(void);

/* Payroll Logic */
void   calculatePayroll(Employee *e);
double getTaxRate(double gross);

/* Search / Validation */
int    findEmployeeById(const char *id);
int    isDuplicateId(const char *id);

/* Input Helpers */
void   clearInputBuffer(void);
int    getValidDouble(const char *prompt, double *out, double minVal);
int    getValidString(const char *prompt, char *out, int maxLen);
char   getYesNo(const char *prompt);

/* Display Helpers */
void   printBox(const char *title, int width);
void   printBoxBottom(int width);
void   printBoxRow(const char *text, int width);
void   printSep(char ch, int count);
void   showMenu(void);
void   showHeader(void);
void   showSummaryBar(void);


/* ╔════════════════════════════════════════════════════════════╗
   ║  MAIN                                                      ║
   ╚════════════════════════════════════════════════════════════╝ */
int main(void)
{
    int choice;

    showHeader();
    loadFromFile();
    showSummaryBar();

    do {
        showMenu();

        if (scanf("%d", &choice) != 1) {
            clearInputBuffer();
            printf(COL_RED "  [!] Invalid input — enter a number 1-8.\n" COL_RESET);
            choice = -1;
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: addEmployee();      break;
            case 2: displayAll();       break;
            case 3: generatePayslip();  break;
            case 4: deleteEmployee();   break;
            case 5: searchEmployee();   break;
            case 6: editEmployee();     break;
            case 7: saveToFile();       break;
            case 8:
                printf(COL_CYAN "\n  Saving data before exit...\n" COL_RESET);
                saveToFile();
                printf(COL_GREEN "  ✔  Goodbye! Have a great day.\n\n" COL_RESET);
                break;
            default:
                printf(COL_RED "  [!] Invalid choice. Enter 1–8.\n" COL_RESET);
        }
    } while (choice != 8);

    return 0;
}

/* ────────────────────────────────────────────────────────────
   showHeader  —  Splash banner shown once at startup
   ──────────────────────────────────────────────────────────── */
void showHeader(void)
{
    printf("\n");
    printf(COL_CYAN "  ╔══════════════════════════════════════════════╗\n");
    printf("  ║" COL_BOLD COL_YELLOW "     EMPLOYEE PAYROLL MANAGEMENT SYSTEM      " COL_RESET COL_CYAN "║\n");
    printf("  ║" COL_DIM "              Terminal Edition v2.0           " COL_RESET COL_CYAN "║\n");
    printf("  ╚══════════════════════════════════════════════╝\n" COL_RESET);
    printf("\n");
}

/* ────────────────────────────────────────────────────────────
   showSummaryBar  —  Quick status line after loading
   ──────────────────────────────────────────────────────────── */
void showSummaryBar(void)
{
    printf(COL_DIM "  ┌─────────────────────────────────────────────┐\n");
    printf("  │  " COL_RESET COL_GREEN "Employees on record : " COL_BOLD "%3d" COL_RESET COL_DIM
           "                       │\n", empCount);
    printf("  │  " COL_RESET COL_BLUE "Max capacity        : " COL_BOLD "%3d" COL_RESET COL_DIM
           "                       │\n", MAX_EMPLOYEES);
    printf("  └─────────────────────────────────────────────┘\n" COL_RESET);
}

/* ────────────────────────────────────────────────────────────
   showMenu  —  Main navigation menu
   ──────────────────────────────────────────────────────────── */
void showMenu(void)
{
    printf("\n" COL_CYAN
           "  ╔══════════════════════════════════════════════╗\n"
           "  ║              MAIN  MENU                      ║\n"
           "  ╠══════════════════════════════════════════════╣\n" COL_RESET);
    printf("  ║  " COL_GREEN "1." COL_RESET "  Add Employee                           " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "2." COL_RESET "  Display All Employees                  " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "3." COL_RESET "  Generate Payslip                       " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "4." COL_RESET "  Delete Employee                        " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "5." COL_RESET "  Search Employee                        " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "6." COL_RESET "  Edit Employee Record                   " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_GREEN "7." COL_RESET "  Save Records to File                   " COL_CYAN "║\n" COL_RESET);
    printf("  ║  " COL_RED   "8." COL_RESET "  Exit                                   " COL_CYAN "║\n" COL_RESET);
    printf(COL_CYAN
           "  ╚══════════════════════════════════════════════╝\n" COL_RESET);
    printf("  " COL_YELLOW "▶  Your choice: " COL_RESET);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  ADD EMPLOYEE                                              ║
   ╚════════════════════════════════════════════════════════════╝ */
void addEmployee(void)
{
    if (empCount >= MAX_EMPLOYEES) {
        printf(COL_RED "\n  [!] Storage full (%d/%d).\n" COL_RESET,
               empCount, MAX_EMPLOYEES);
        return;
    }

    Employee e;
    memset(&e, 0, sizeof(e));

    printf("\n" COL_CYAN
           "  ┌─────────────────────────────────────────────┐\n"
           "  │              ADD NEW EMPLOYEE               │\n"
           "  └─────────────────────────────────────────────┘\n" COL_RESET);

    /* ── Employee ID ── */
    while (1) {
        if (!getValidString("  Employee ID  (e.g. EMP001) : ", e.empId, ID_LEN)) {
            printf(COL_RED "  [!] ID cannot be empty.\n" COL_RESET);
            continue;
        }
        for (int i = 0; e.empId[i]; i++)
            e.empId[i] = toupper((unsigned char)e.empId[i]);
        if (isDuplicateId(e.empId)) {
            printf(COL_RED "  [!] ID '%s' already exists.\n" COL_RESET, e.empId);
            continue;
        }
        break;
    }

    /* ── Name ── */
    while (!getValidString("  Full Name                  : ", e.name, NAME_LEN))
        printf(COL_RED "  [!] Name cannot be empty.\n" COL_RESET);

    /* ── Department ── */
    while (!getValidString("  Department                 : ", e.dept, DEPT_LEN))
        printf(COL_RED "  [!] Department cannot be empty.\n" COL_RESET);

    /* ── Basic Pay ── */
    if (!getValidDouble("  Basic Pay (Rs.)            : ", &e.basicPay, 1.0)) {
        printf(COL_YELLOW "  [!] Cancelled.\n" COL_RESET); return;
    }

    /* ── OT Hours ── */
    if (!getValidDouble("  Overtime Hours             : ", &e.otHours, 0.0)) {
        printf(COL_YELLOW "  [!] Cancelled.\n" COL_RESET); return;
    }

    calculatePayroll(&e);
    employees[empCount++] = e;

    printf(COL_GREEN
           "\n  ╔══════════════════════════════════════════════╗\n"
           "  ║  ✔  Employee added successfully!             ║\n"
           "  ╠══════════════════════════════════════════════╣\n" COL_RESET);
    printf("  ║  ID       : %-33s" COL_GREEN "║\n" COL_RESET, e.empId);
    printf("  ║  Name     : %-33s" COL_GREEN "║\n" COL_RESET, e.name);
    printf("  ║  Net Pay  : Rs. %-29.2f" COL_GREEN "║\n" COL_RESET, e.netPay);
    printf(COL_GREEN "  ╚══════════════════════════════════════════════╝\n" COL_RESET);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  DISPLAY ALL EMPLOYEES                                     ║
   ╚════════════════════════════════════════════════════════════╝ */
void displayAll(void)
{
    if (empCount == 0) {
        printf(COL_YELLOW "\n  [!] No records found.\n" COL_RESET);
        return;
    }

    int W = 98;
    printf("\n" COL_CYAN);
    printSep('═', W);
    printf("  %-10s %-20s %-16s %11s %7s %12s %7s %12s\n",
           "EMP ID", "NAME", "DEPARTMENT", "BASIC (Rs.)",
           "OT Hrs", "GROSS (Rs.)", "TAX %", "NET (Rs.)");
    printSep('─', W);
    printf(COL_RESET);

    double totalNet = 0.0;
    for (int i = 0; i < empCount; i++) {
        Employee *e = &employees[i];
        /* Alternate row shading */
        if (i % 2 == 0) printf(COL_DIM);
        printf("  %-10s %-20s %-16s %11.2f %7.1f %12.2f %6.0f%% %12.2f\n",
               e->empId, e->name, e->dept,
               e->basicPay, e->otHours,
               e->grossPay, e->taxRate * 100.0, e->netPay);
        printf(COL_RESET);
        totalNet += e->netPay;
    }

    printf(COL_CYAN);
    printSep('═', W);
    printf(COL_RESET);
    printf("  " COL_BOLD "Total Employees: %d" COL_RESET
           "     " COL_BOLD "Total Net Payroll: Rs. %.2f\n" COL_RESET,
           empCount, totalNet);
    printf(COL_CYAN);
    printSep('─', W);
    printf(COL_RESET);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  GENERATE PAYSLIP                                          ║
   ╚════════════════════════════════════════════════════════════╝ */
void generatePayslip(void)
{
    if (empCount == 0) {
        printf(COL_YELLOW "\n  [!] No records available.\n" COL_RESET); return;
    }

    char id[ID_LEN];
    printf("\n  Enter Employee ID : ");
    if (!getValidString("", id, ID_LEN)) {
        printf(COL_RED "  [!] No ID entered.\n" COL_RESET); return;
    }
    for (int i = 0; id[i]; i++) id[i] = toupper((unsigned char)id[i]);

    int idx = findEmployeeById(id);
    if (idx == -1) {
        printf(COL_RED "  [!] ID '%s' not found.\n" COL_RESET, id); return;
    }
    Employee *e = &employees[idx];

    int W = 54;
    printf("\n" COL_MAGENTA);
    printSep('═', W);
    printf("  %-*s\n", W - 3, "");
    printf("  %*s" COL_BOLD "SALARY SLIP" COL_RESET COL_MAGENTA "%*s\n",
           (W - 14) / 2, "", (W - 14) / 2, "");
    printf("  %-*s\n", W - 3, "");
    printSep('═', W);
    printf(COL_RESET);

    printf("  Employee ID   : " COL_BOLD "%s\n" COL_RESET, e->empId);
    printf("  Employee Name : " COL_BOLD "%s\n" COL_RESET, e->name);
    printf("  Department    : " COL_BOLD "%s\n" COL_RESET, e->dept);
    printf(COL_MAGENTA); printSep('─', W); printf(COL_RESET);

    printf(COL_BOLD "  EARNINGS\n" COL_RESET);
    printf("  %-30s Rs. %10.2f\n", "Basic Pay", e->basicPay);
    printf("  %-30s Rs. %10.2f\n",
           "Overtime (hrs x Rs.100/hr)", e->otPay);
    printf("  %-30s     %6.1f hrs\n", "Overtime Hours", e->otHours);
    printf(COL_MAGENTA); printSep('─', W); printf(COL_RESET);
    printf(COL_BOLD "  %-30s Rs. %10.2f\n" COL_RESET, "GROSS PAY", e->grossPay);
    printf(COL_MAGENTA); printSep('─', W); printf(COL_RESET);

    printf(COL_BOLD "  DEDUCTIONS\n" COL_RESET);
    printf("  Income Tax (%.0f%%)             Rs. %10.2f\n",
           e->taxRate * 100.0, e->taxAmount);
    printf(COL_MAGENTA); printSep('─', W); printf(COL_RESET);
    printf(COL_BOLD "  %-30s Rs. %10.2f\n" COL_RESET, "TOTAL DEDUCTIONS", e->taxAmount);

    printf(COL_CYAN); printSep('═', W); printf(COL_RESET);
    printf(COL_BOLD COL_GREEN "  %-30s Rs. %10.2f\n" COL_RESET, "NET PAY", e->netPay);
    printf(COL_CYAN); printSep('═', W); printf(COL_RESET);
    printf(COL_DIM
           "  ** System-generated. No signature required. **\n" COL_RESET);
    printf(COL_MAGENTA); printSep('─', W); printf(COL_RESET);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  DELETE EMPLOYEE                                           ║
   ╚════════════════════════════════════════════════════════════╝ */
void deleteEmployee(void)
{
    if (empCount == 0) {
        printf(COL_YELLOW "\n  [!] No records to delete.\n" COL_RESET); return;
    }

    char id[ID_LEN];
    printf("\n  Employee ID to delete : ");
    if (!getValidString("", id, ID_LEN)) {
        printf(COL_RED "  [!] No ID entered.\n" COL_RESET); return;
    }
    for (int i = 0; id[i]; i++) id[i] = toupper((unsigned char)id[i]);

    int idx = findEmployeeById(id);
    if (idx == -1) {
        printf(COL_RED "  [!] ID '%s' not found.\n" COL_RESET, id); return;
    }

    printf(COL_YELLOW "  Found: %s  (%s)\n" COL_RESET,
           employees[idx].name, employees[idx].empId);

    if (getYesNo("  Confirm delete? (y/n) : ") != 'y') {
        printf("  Deletion cancelled.\n"); return;
    }

    /* Shift left */
    for (int i = idx; i < empCount - 1; i++)
        employees[i] = employees[i + 1];
    empCount--;

    printf(COL_GREEN "  ✔  Record deleted. Remaining: %d\n" COL_RESET, empCount);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  SEARCH EMPLOYEE                                           ║
   ╚════════════════════════════════════════════════════════════╝ */
void searchEmployee(void)
{
    if (empCount == 0) {
        printf(COL_YELLOW "\n  [!] No records.\n" COL_RESET); return;
    }

    char id[ID_LEN];
    printf("\n  Employee ID to search : ");
    if (!getValidString("", id, ID_LEN)) {
        printf(COL_RED "  [!] No ID entered.\n" COL_RESET); return;
    }
    for (int i = 0; id[i]; i++) id[i] = toupper((unsigned char)id[i]);

    int idx = findEmployeeById(id);
    if (idx == -1) {
        printf(COL_RED "  [!] ID '%s' not found.\n" COL_RESET, id); return;
    }

    Employee *e = &employees[idx];
    printf(COL_CYAN "\n  ┌─────────────────────────────────────────────┐\n");
    printf("  │            EMPLOYEE DETAILS                 │\n");
    printf("  └─────────────────────────────────────────────┘\n" COL_RESET);
    printf("  ID          : " COL_BOLD "%s\n" COL_RESET, e->empId);
    printf("  Name        : " COL_BOLD "%s\n" COL_RESET, e->name);
    printf("  Department  : " COL_BOLD "%s\n" COL_RESET, e->dept);
    printf("  Basic Pay   : Rs. %.2f\n", e->basicPay);
    printf("  OT Hours    : %.1f hrs\n", e->otHours);
    printf("  Gross Pay   : Rs. %.2f\n", e->grossPay);
    printf("  Tax (%.0f%%)   : Rs. %.2f\n", e->taxRate * 100.0, e->taxAmount);
    printf("  " COL_BOLD COL_GREEN "Net Pay     : Rs. %.2f\n" COL_RESET, e->netPay);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  EDIT EMPLOYEE                                             ║
   ╚════════════════════════════════════════════════════════════╝ */
void editEmployee(void)
{
    if (empCount == 0) {
        printf(COL_YELLOW "\n  [!] No records to edit.\n" COL_RESET); return;
    }

    char id[ID_LEN];
    printf("\n  Employee ID to edit : ");
    if (!getValidString("", id, ID_LEN)) {
        printf(COL_RED "  [!] No ID entered.\n" COL_RESET); return;
    }
    for (int i = 0; id[i]; i++) id[i] = toupper((unsigned char)id[i]);

    int idx = findEmployeeById(id);
    if (idx == -1) {
        printf(COL_RED "  [!] ID '%s' not found.\n" COL_RESET, id); return;
    }

    Employee *e = &employees[idx];
    printf(COL_YELLOW "\n  Editing: %s (%s)\n" COL_RESET, e->name, e->empId);
    printf(COL_DIM "  (Press Enter to keep current value)\n\n" COL_RESET);

    char buf[NAME_LEN];
    double val;

    /* Name */
    printf("  Name [%s]: ", e->name);
    if (getValidString("", buf, NAME_LEN)) strncpy(e->name, buf, NAME_LEN);

    /* Department */
    printf("  Department [%s]: ", e->dept);
    if (getValidString("", buf, DEPT_LEN)) strncpy(e->dept, buf, DEPT_LEN);

    /* Basic Pay */
    printf("  Basic Pay [%.2f]: ", e->basicPay);
    if (getValidDouble("", &val, 1.0)) e->basicPay = val;

    /* OT Hours */
    printf("  OT Hours [%.1f]: ", e->otHours);
    if (getValidDouble("", &val, 0.0)) e->otHours = val;

    calculatePayroll(e);
    printf(COL_GREEN "\n  ✔  Record updated. New Net Pay: Rs. %.2f\n" COL_RESET, e->netPay);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  PAYROLL CALCULATION                                       ║
   ╚════════════════════════════════════════════════════════════╝ */
void calculatePayroll(Employee *e)
{
    e->otPay     = e->otHours * OT_RATE;
    e->grossPay  = e->basicPay + e->otPay;
    e->taxRate   = getTaxRate(e->grossPay);
    e->taxAmount = e->grossPay * e->taxRate;
    e->netPay    = e->grossPay - e->taxAmount;
}

double getTaxRate(double gross)
{
    if (gross <= 30000.0) return 0.05;
    if (gross <= 60000.0) return 0.10;
    return 0.15;
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  FILE I/O                                                  ║
   ╚════════════════════════════════════════════════════════════╝ */
void saveToFile(void)
{
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        printf(COL_RED "  [!] Cannot open '%s' for writing.\n" COL_RESET, DATA_FILE);
        return;
    }
    fwrite(&empCount, sizeof(int),      1,        fp);
    fwrite(employees,  sizeof(Employee), empCount, fp);
    fclose(fp);
    printf(COL_GREEN "  ✔  %d record(s) saved to '%s'.\n" COL_RESET,
           empCount, DATA_FILE);
}

void loadFromFile(void)
{
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) {
        printf(COL_DIM "  [Info] No saved file found — starting fresh.\n" COL_RESET);
        return;
    }
    int savedCount = 0;
    if (fread(&savedCount, sizeof(int), 1, fp) != 1 ||
        savedCount < 0 || savedCount > MAX_EMPLOYEES) {
        printf(COL_RED "  [!] Corrupted file — starting fresh.\n" COL_RESET);
        fclose(fp); return;
    }
    empCount = (int)fread(employees, sizeof(Employee), savedCount, fp);
    fclose(fp);
    printf(COL_GREEN "  ✔  %d record(s) loaded from '%s'.\n" COL_RESET,
           empCount, DATA_FILE);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  SEARCH HELPERS                                            ║
   ╚════════════════════════════════════════════════════════════╝ */
int findEmployeeById(const char *id)
{
    for (int i = 0; i < empCount; i++)
        if (strcmp(employees[i].empId, id) == 0) return i;
    return -1;
}

int isDuplicateId(const char *id) { return findEmployeeById(id) != -1; }

/* ╔════════════════════════════════════════════════════════════╗
   ║  INPUT HELPERS                                             ║
   ╚════════════════════════════════════════════════════════════╝ */

/*  getValidDouble ─ safe numeric input with minimum value check.
    Returns 1 on success, 0 if user types 'q' to cancel.         */
int getValidDouble(const char *prompt, double *out, double minVal)
{
    char buf[64];
    while (1) {
        if (prompt && prompt[0]) printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        buf[strcspn(buf, "\n")] = '\0';
        if (!buf[0]) return 0;                 /* empty → skip (for edit) */
        if (buf[0]=='q'||buf[0]=='Q') return 0;

        char *ep;
        double v = strtod(buf, &ep);
        if (ep == buf || *ep != '\0') {
            printf(COL_RED "  [!] Not a valid number (or 'q' to cancel).\n" COL_RESET);
            continue;
        }
        if (v < minVal) {
            printf(COL_RED "  [!] Must be >= %.2f.\n" COL_RESET, minVal);
            continue;
        }
        *out = v;
        return 1;
    }
}

/*  getValidString ─ reads a non-empty trimmed string.
    Returns 1 on success, 0 on empty / whitespace-only.          */
int getValidString(const char *prompt, char *out, int maxLen)
{
    if (prompt && prompt[0]) printf("%s", prompt);
    if (!fgets(out, maxLen, stdin)) { out[0]='\0'; return 0; }
    out[strcspn(out, "\n")] = '\0';
    for (int i = 0; out[i]; i++)
        if (!isspace((unsigned char)out[i])) return 1;
    return 0;
}

/*  getYesNo ─ prompts for y/n and returns lowercased answer.    */
char getYesNo(const char *prompt)
{
    printf("%s", prompt);
    char c; scanf(" %c", &c); clearInputBuffer();
    return (char)tolower((unsigned char)c);
}

/*  clearInputBuffer ─ discard chars left in stdin after scanf.  */
void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ╔════════════════════════════════════════════════════════════╗
   ║  DISPLAY HELPERS                                           ║
   ╚════════════════════════════════════════════════════════════╝ */
void printSep(char ch, int count)
{
    for (int i = 0; i < count; i++) putchar(ch);
    putchar('\n');
}
