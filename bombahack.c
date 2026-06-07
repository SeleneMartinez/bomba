#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>

int main() {
    pid_t hijo = fork();

    if (hijo == 0) {
        // --- PROCESO HIJO ---
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
       
        // Ejecución interactiva pura por terminal
        execl("./Bomba", "Bomba", NULL);
        perror("Error al ejecutar ./Bomba");
        exit(1);
    }
    else {
        // --- PROCESO PADRE ---
        int estado;
        struct user_regs_struct regs;
       
        // 1. Esperamos el freno inicial de Linux tras el fork/exec
        wait(&estado);

        // 2. Colocamos el Breakpoint en el CMP (0x176e)
        uint64_t dir_breakpoint = 0x176e;
        uint64_t original_codigo = ptrace(PTRACE_PEEKTEXT, hijo, (void*)dir_breakpoint, NULL);
        uint64_t codigo_breakp = (original_codigo & 0xffffffffffffff00) | 0xCC;
        ptrace(PTRACE_POKETEXT, hijo, (void*)dir_breakpoint, (void*)(uintptr_t)codigo_breakp);
       
        printf("[OK] Buscador interactivo listo. Corriendo la bomba...\n");
        printf("👉 Introduce cualquier número y palabra si te lo pide (Ej: 0 test):\n");
        printf("--------------------------------------------------\n");

        // 3. Liberamos a la bomba para que interactúe contigo en la terminal
        ptrace(PTRACE_CONT, hijo, NULL, NULL);
       
        // 4. El Padre duerme hasta que tú escribas en la terminal y des Enter
        wait(&estado);

        printf("\n[💥] ¡BREAKPOINT ALCANZADO EN LA COMPROBACIÓN!\n");

        // 5. Extraemos los registros actuales de la CPU del hijo
        ptrace(PTRACE_GETREGS, hijo, NULL, &regs);
       
        // CORRECCIÓN 1: Leer el valor real del registro r15 en ese instante
        int clave_entera_real = (int)regs.r15;

        printf("==================================================\n");
        printf("🔑 CLAVES REALES EXTRAÍDAS DE LA MEMORIA:\n");
        printf("==================================================\n");
        printf(" -> Clave Entera de esta ejecución: %d\n", clave_entera_real);
        printf(" -> Strings ocultos detectados en la Pila: ");

        // CORRECCIÓN 2: Escaneo de texto limpio y ordenado
        int ultimo_fue_caracter = 0;
        for (int i = 0; i < 40; i++) {
            uint64_t datos = ptrace(PTRACE_PEEKTEXT, hijo, (void*)(regs.rsp + (i * 8)), NULL);
           
            // Analizamos byte por byte respetando el orden de memoria
            for (int j = 0; j < 8; j++) {
                unsigned char c = (datos >> (j * 8)) & 0xFF;
               
                if (isalpha(c) || c == '_' || c == '-' || (c >= '0' && c <= '9')) {
                    putchar(c);
                    ultimo_fue_caracter = 1;
                } else {
                    // Si encontramos un nulo o basura tras un texto válido, ponemos un solo espacio
                    if (ultimo_fue_caracter) {
                        putchar(' ');
                        ultimo_fue_caracter = 0;
                    }
                }
            }
        }
        printf("\n==================================================\n\n");

        // CORRECCIÓN 3: Modificar la memoria del usuario para salvar la bomba
        // Colocamos la clave entera real en 0x44(%rsp) para burlar el 'cmp' de la fase
        uint64_t dir_pila_usuario = regs.rsp + 0x44;
        ptrace(PTRACE_POKEDATA, hijo, (void*)dir_pila_usuario, (void*)(long)clave_entera_real);
        printf("[+] Parche aplicado: Se inyectó el número %d en tu entrada.\n", clave_entera_real);

        // 6. Limpieza final estándar y restauración del flujo
        ptrace(PTRACE_POKETEXT, hijo, (void*)dir_breakpoint, (void*)(uintptr_t)original_codigo);
        regs.rip = regs.rip - 1; // Hacer retroceder al procesador 1 byte (el tamaño de 0xCC)
        ptrace(PTRACE_SETREGS, hijo, NULL, &regs);
       
        printf("[+] Continuando ejecución de la bomba...\n");
        ptrace(PTRACE_CONT, hijo, NULL, NULL);
       
        // Esperamos a que la bomba finalice o imprima el resultado de éxito
        wait(&estado);
    }
    return 0;
}
