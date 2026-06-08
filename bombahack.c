//break *(0x555555554000 + 0x176e)
//set {int}($rsp + 0x44) = $r15d
//break *(0x555555554000 + 0x17c8)
//set {float}($rsp + 0x48) = $xmm3.v4_float[0]
//break *(0x555555554000 + 0x182f) 
//set {int}($rsp + 0x44) = $ebx
//break *(0x555555554000 + 0x18ae)
//(gdb) call (char *) strcpy($rdi, $rsi)
//break *(0x555555554000 + 0x1908)

//set {int}($rsp + 0x44) = $ebx
//break *(0x555555554000 + 0x195f)
//set {float}($rsp + 0x4c) = $xmm4.v4_float[0]
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/ptrace.h>
// #include <sys/wait.h>
// #include <sys/user.h>
// #include <sys/types.h>

// #define BOMB_PATH "/home/vboxuser/Downloads/bomba"

// // Offsets reales verificados de tu binario
// #define OFFSET_FASE1_CMP   0x176e  // cmp %r15d, 0x44(%rsp)
// #define OFFSET_FASE2_FLOAT 0x17c8  // ucomiss 0x48(%rsp), %xmm3
// #define OFFSET_FASE2_CMP   0x182f  // cmp %ebx, 0x44(%rsp)
// #define OFFSET_FASE3_TEST  0x18b3  // test %eax, %eax (Verificación String de Fase 3)
// #define OFFSET_FASE4_CMP   0x1911  // cmp %ebx, 0x44(%rsp) (Verificación Entero de Fase 4)

// int main() {
//     pid_t child;
//     int status;
//     struct user_regs_struct regs;
//     struct user_fpregs_struct fpregs; 
//     long base_address = 0;

//     child = fork();
//     if (child == 0) {
//         // ====================================================
//         // PROCESO HIJO: LA BOMBA REAL (INTERACTIVA)
//         // ====================================================
//         ptrace(PTRACE_TRACEME, 0, NULL, NULL);
//         execl(BOMB_PATH, BOMB_PATH, NULL);
//         perror("[-] Error al ejecutar la bomba");
//         exit(1);
//     } 

//     // ====================================================
//     // PROCESO PADRE: TU ASISTENTE EN C
//     // ====================================================
//     waitpid(child, &status, 0);

//     // Obtener la dirección base dinámica (ASLR)
//     char maps_path[64];
//     sprintf(maps_path, "/proc/%d/maps", child);
//     FILE *maps = fopen(maps_path, "r");
//     if (maps) {
//         fscanf(maps, "%lx", &base_address);
//         fclose(maps);
//     } else {
//         perror("[-] No se pudo obtener la dirección base");
//         return 1;
//     }

//     unsigned long addr_fase1 = base_address + OFFSET_FASE1_CMP;
//     unsigned long addr_fase2_float = base_address + OFFSET_FASE2_FLOAT;
//     unsigned long addr_fase2_cmp = base_address + OFFSET_FASE2_CMP;
//     unsigned long addr_fase3 = base_address + OFFSET_FASE3_TEST;
//     unsigned long addr_fase4 = base_address + OFFSET_FASE4_CMP;

//     printf("[+] Dirección base de la bomba: 0x%lx\n", base_address);

//     // --------------------------------------------------------
//     // FASE 1 INTERACTIVA
//     // --------------------------------------------------------
//     long orig_fase1 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase1, NULL);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase1, (void*)((orig_fase1 & ~0xFF) | 0xCC));

//     ptrace(PTRACE_CONT, child, NULL, NULL);
//     waitpid(child, &status, 0); 

//     ptrace(PTRACE_GETREGS, child, NULL, &regs);
//     unsigned int secreto_fase1 = regs.r15 & 0xFFFFFFFF;
//     unsigned long stack_f1 = regs.rsp + 0x44;
//     long tu_entrada_f1 = ptrace(PTRACE_PEEKDATA, child, (void*)stack_f1, NULL) & 0xFFFFFFFF;

//     printf("\n==================================================\n");
//     printf("[*] ASISTENTE FASE 1:\n");
//     printf("[*] El secreto calculado en %%r15d era: %u\n", secreto_fase1);
//     printf("[*] Tu número ingresado fue: %ld\n", tu_entrada_f1);
//     printf("==================================================\n\n");

//     regs.r15 = tu_entrada_f1;
//     regs.rip = addr_fase1;
//     ptrace(PTRACE_SETREGS, child, NULL, &regs);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase1, (void*)orig_fase1);

//     // --------------------------------------------------------
//     // FASE 2 INTERACTIVA (PARTE FLOAT)
//     // --------------------------------------------------------
//     long orig_fase2_float = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase2_float, NULL);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_float, (void*)((orig_fase2_float & ~0xFF) | 0xCC));

//     ptrace(PTRACE_CONT, child, NULL, NULL);
//     waitpid(child, &status, 0); 

//     ptrace(PTRACE_GETFPREGS, child, NULL, &fpregs);
//     float *xmm3_ptr = (float *)&fpregs.xmm_space[3 * 4]; 
//     float secreto_float = *xmm3_ptr;

//     ptrace(PTRACE_GETREGS, child, NULL, &regs);
//     unsigned long stack_float_target = regs.rsp + 0x48;
//     long bits_tu_float = ptrace(PTRACE_PEEKDATA, child, (void*)stack_float_target, NULL);
//     float *tu_entrada_float = (float *)&bits_tu_float;

//     printf("\n==================================================\n");
//     printf("[*] ASISTENTE FASE 2 (Validación Float):\n");
//     printf("[*] El float secreto en %%xmm3 es: %f\n", secreto_float);
//     printf("[*] Tu float ingresado manualmente fue: %f\n", *tu_entrada_float);
//     printf("==================================================\n\n");

//     unsigned int *float_bits = (unsigned int *)&secreto_float;
//     ptrace(PTRACE_POKEDATA, child, (void*)stack_float_target, (void*)(unsigned long)*float_bits);

//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_float, (void*)orig_fase2_float);
//     regs.rip = addr_fase2_float;
//     ptrace(PTRACE_SETREGS, child, NULL, &regs);

//     // --------------------------------------------------------
//     // FASE 2 AUTOMÁTICA (PARTE ENTERA)
//     // --------------------------------------------------------
//     long orig_fase2_cmp = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase2_cmp, NULL);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_cmp, (void*)((orig_fase2_cmp & ~0xFF) | 0xCC));

//     ptrace(PTRACE_CONT, child, NULL, NULL);
//     waitpid(child, &status, 0); 

//     ptrace(PTRACE_GETREGS, child, NULL, &regs);
//     unsigned int secreto_fase2_int = regs.rbx & 0xFFFFFFFF;

//     unsigned long stack_int_target = regs.rsp + 0x44;
//     ptrace(PTRACE_POKEDATA, child, (void*)stack_int_target, (void*)(unsigned long)secreto_fase2_int);

//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_cmp, (void*)orig_fase2_cmp);
//     regs.rip = addr_fase2_cmp;
//     ptrace(PTRACE_SETREGS, child, NULL, &regs);

//     // --------------------------------------------------------
//     // FASE 3 AUTOMÁTICA (PUENTE DE STRING PARA LLEGAR A LA 4)
//     // --------------------------------------------------------
//     long orig_fase3 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase3, NULL);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase3, (void*)((orig_fase3 & ~0xFF) | 0xCC));

//     ptrace(PTRACE_CONT, child, NULL, NULL);
    
//     // Aquí la bomba te pedirá el texto de la Fase 3. Escribe cualquier cosa (ej: "hola")
//     waitpid(child, &status, 0); 

//     // Al presionar Enter, forzamos de forma invisible que strcmp haya dado correcto (EAX = 0)
//     ptrace(PTRACE_GETREGS, child, NULL, &regs);
//     regs.rax = 0; 
//     regs.rip = addr_fase3;
//     ptrace(PTRACE_SETREGS, child, NULL, &regs);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase3, (void*)orig_fase3);

//     // --------------------------------------------------------
//     // FASE 4 INTERACTIVA (Dirección 1911)
//     // --------------------------------------------------------
//     // Inyectamos el breakpoint en la comparación final de la Fase 4
//     long orig_fase4 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase4, NULL);
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase4, (void*)((orig_fase4 & ~0xFF) | 0xCC));

//     ptrace(PTRACE_CONT, child, NULL, NULL);

//     // Aquí la bomba avanzará y te imprimirá: "Fase 4: Ingresa el codigo..." (o similar)
//     // El script se detiene y espera a que TÚ ingreses el entero que quieras.
//     waitpid(child, &status, 0); 

//     // ¡Acabas de presionar Enter! Leemos el registro %ebx para ver la clave matemática real
//     ptrace(PTRACE_GETREGS, child, NULL, &regs);
//     unsigned int secreto_fase4 = regs.rbx & 0xFFFFFFFF;

//     // Leemos la pila en 0x44(%rsp) para ver qué número inventaste tú
//     unsigned long stack_fase4_target = regs.rsp + 0x44;
//     long tu_entrada_f4 = ptrace(PTRACE_PEEKDATA, child, (void*)stack_fase4_target, NULL) & 0xFFFFFFFF;

//     printf("\n==================================================\n");
//     printf("[*] ASISTENTE FASE 4 (Validación Entero Final):\n");
//     printf("[*] La clave matemática en %%ebx es: %u\n", secreto_fase4);
//     printf("[*] Tu número ingresado manualmente fue: %ld\n", tu_entrada_f4);
//     printf("==================================================\n\n");

//     // Parchamos la pila con el valor real para perdonarte la vida y pasar de fase
//     ptrace(PTRACE_POKEDATA, child, (void*)stack_fase4_target, (void*)(unsigned long)secreto_fase4);

//     // Restauramos la instrucción original
//     ptrace(PTRACE_POKETEXT, child, (void*)addr_fase4, (void*)orig_fase4);
//     regs.rip = addr_fase4;
//     ptrace(PTRACE_SETREGS, child, NULL, &regs);

//     // ====================================================
//     // PASO LIMPIO A LA FASE 5
//     // ====================================================
//     printf("[+] Fase 4 resuelta de forma interactiva.\n");
//     printf("[+] Desacoplando asistente. ¡Entrando a la Fase 5!...\n\n");
//     ptrace(PTRACE_DETACH, child, NULL, NULL);

//     wait(NULL);
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <string.h>

#define BOMB_PATH "/home/vboxuser/Downloads/bomba"

// Offsets reales y verificados de tu binario
#define OFFSET_FASE1_CMP   0x176e  // cmp %r15d, 0x44(%rsp)
#define OFFSET_FASE2_FLOAT 0x17c8  // ucomiss 0x48(%rsp), %xmm3
#define OFFSET_FASE3_CMP   0x182f  // cmp %ebx, 0x44(%rsp)
#define OFFSET_FASE4_STR   0x18ae  // call strcmp (Fase 4 - Validación Texto)
#define OFFSET_FASE5_CMP   0x1911  // cmp %ebx, 0x44(%rsp) (Fase 5 - Validación Entero)
#define OFFSET_FASE6_FLOAT 0x195f  // ucomiss 0x4c(%rsp), %xmm4 (Fase 6 - Validación Float)

// Función auxiliar para clonar strings entre procesos mediante ptrace
void ptrace_strcpy(pid_t child, unsigned long dest, unsigned long src) {
    char ch;
    do {
        long word = ptrace(PTRACE_PEEKDATA, child, (void*)src, NULL);
        ch = word & 0xFF;
        
        long dest_word = ptrace(PTRACE_PEEKDATA, child, (void*)dest, NULL);
        dest_word = (dest_word & ~0xFF) | ch;
        ptrace(PTRACE_POKEDATA, child, (void*)dest, (void*)dest_word);
        
        dest++;
        src++;
    } while (ch != '\0');
}

int main() {
    pid_t child;
    int status;
    struct user_regs_struct regs;
    struct user_fpregs_struct fpregs; 
    long base_address = 0;

    child = fork();
    if (child == 0) {
        // ====================================================
        // PROCESO HIJO: EJECUTA LA BOMBA ORIGINAL
        // ====================================================
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(BOMB_PATH, BOMB_PATH, NULL);
        perror("[-] Error al ejecutar la bomba");
        exit(1);
    } 

    // ====================================================
    // PROCESO PADRE: MONITOR / ASISTENTE EN CALIENTE
    // ====================================================
    waitpid(child, &status, 0);

    // Resolver dirección base dinámica (ASLR) del proceso hijo
    char maps_path[64];
    sprintf(maps_path, "/proc/%d/maps", child);
    FILE *maps = fopen(maps_path, "r");
    if (maps) {
        fscanf(maps, "%lx", &base_address);
        fclose(maps);
    } else {
        perror("[-] No se pudo obtener la dirección base");
        return 1;
    }

    // Calcular direcciones de memoria en tiempo de ejecución
    unsigned long addr_fase1 = base_address + OFFSET_FASE1_CMP;
    unsigned long addr_fase2_float = base_address + OFFSET_FASE2_FLOAT;
    unsigned long addr_fase3 = base_address + OFFSET_FASE3_CMP;
    unsigned long addr_fase4 = base_address + OFFSET_FASE4_STR;
    unsigned long addr_fase5 = base_address + OFFSET_FASE5_CMP;
    unsigned long addr_fase6 = base_address + OFFSET_FASE6_FLOAT;

    printf("[+] Dirección base de la bomba: 0x%lx\n", base_address);

    // --------------------------------------------------------
    // FASE 1 INTERACTIVA
    // --------------------------------------------------------
    long orig_fase1 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase1, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase1, (void*)((orig_fase1 & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned long stack_f1 = regs.rsp + 0x44;
    long tu_entrada_f1 = ptrace(PTRACE_PEEKDATA, child, (void*)stack_f1, NULL) & 0xFFFFFFFF;
    
    regs.r15 = tu_entrada_f1; 
    regs.rip = addr_fase1;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase1, (void*)orig_fase1);

    // --------------------------------------------------------
    // FASE 2 INTERACTIVA (FLOAT)
    // --------------------------------------------------------
    long orig_fase2_float = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase2_float, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_float, (void*)((orig_fase2_float & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETFPREGS, child, NULL, &fpregs);
    float *xmm3_ptr = (float *)&fpregs.xmm_space[3 * 4]; 
    float secreto_float = *xmm3_ptr;
    
    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned long stack_float_target = regs.rsp + 0x48;
    unsigned int *float_bits = (unsigned int *)&secreto_float;
    ptrace(PTRACE_POKEDATA, child, (void*)stack_float_target, (void*)(unsigned long)*float_bits); 
    
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase2_float, (void*)orig_fase2_float);
    regs.rip = addr_fase2_float;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    // --------------------------------------------------------
    // FASE 3 INTERACTIVA (ENTEROS)
    // --------------------------------------------------------
    long orig_fase3 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase3, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase3, (void*)((orig_fase3 & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned int secreto_fase3_int = regs.rbx & 0xFFFFFFFF;
    unsigned long stack_int_target = regs.rsp + 0x44;
    ptrace(PTRACE_POKEDATA, child, (void*)stack_int_target, (void*)(unsigned long)secreto_fase3_int); 
    
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase3, (void*)orig_fase3);
    regs.rip = addr_fase3;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    // --------------------------------------------------------
    // FASE 4 INTERACTIVA (TEXTO / STRING CLONATION)
    // --------------------------------------------------------
    long orig_fase4 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase4, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase4, (void*)((orig_fase4 & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned long tu_buffer_str = regs.rdi;     
    unsigned long secreto_bomb_str = regs.rsi;  

    ptrace_strcpy(child, tu_buffer_str, secreto_bomb_str); 
    printf("[*] ASISTENTE C: Clonado el string secreto en la Fase 4.\n");

    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase4, (void*)orig_fase4);
    regs.rip = addr_fase4;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    // --------------------------------------------------------
    // FASE 5 INTERACTIVA (ENTEROS)
    // --------------------------------------------------------
    long orig_fase5 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase5, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase5, (void*)((orig_fase5 & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned int secreto_fase5_int = regs.rbx & 0xFFFFFFFF;
    unsigned long stack_f5_target = regs.rsp + 0x44;
    
    ptrace(PTRACE_POKEDATA, child, (void*)stack_f5_target, (void*)(unsigned long)secreto_fase5_int); 
    printf("[*] ASISTENTE C: Parchada la pila en Fase 5 con el entero: %u\n", secreto_fase5_int);

    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase5, (void*)orig_fase5);
    regs.rip = addr_fase5;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    // --------------------------------------------------------
    // FASE 6 INTERACTIVA (FLOAT)
    // --------------------------------------------------------
    long orig_fase6 = ptrace(PTRACE_PEEKTEXT, child, (void*)addr_fase6, NULL);
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase6, (void*)((orig_fase6 & ~0xFF) | 0xCC));
    ptrace(PTRACE_CONT, child, NULL, NULL);
    waitpid(child, &status, 0); 

    ptrace(PTRACE_GETFPREGS, child, NULL, &fpregs);
    float *xmm4_ptr = (float *)&fpregs.xmm_space[4 * 4]; 
    float secreto_f6_float = *xmm4_ptr;

    ptrace(PTRACE_GETREGS, child, NULL, &regs);
    unsigned long stack_f6_target = regs.rsp + 0x4c;
    unsigned int *f6_bits = (unsigned int *)&secreto_f6_float;
    
    ptrace(PTRACE_POKEDATA, child, (void*)stack_f6_target, (void*)(unsigned long)*f6_bits); 
    printf("[*] ASISTENTE C: Parchada la pila en Fase 6 con el float: %f\n", secreto_f6_float);

    // Corregido de addr_f6 a addr_fase6
    ptrace(PTRACE_POKETEXT, child, (void*)addr_fase6, (void*)orig_fase6); 
    regs.rip = addr_fase6;
    ptrace(PTRACE_SETREGS, child, NULL, &regs);

    // ====================================================
    // FIN DEL ATAQUE: SE INDEPENDIZA EL PROCESO
    // ====================================================
    printf("[+] ¡Fases resueltas con éxito manipulando la memoria!\n");
    printf("[+] Desacoplando exploit...\n\n");
    ptrace(PTRACE_DETACH, child, NULL, NULL);

    wait(NULL);
    return 0;
}
