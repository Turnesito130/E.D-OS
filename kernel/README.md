# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un **kernel propio e independiente**. No utilizará el kernel Linux para arrancar ni para funcionar.

Linux se estudia como referencia de ingeniería. Cualquier reutilización de código será selectiva y requerirá revisar dependencias, procedencia, licencia y obligaciones de redistribución antes de incorporarlo.

## Prototipo de kernel 0.0.2

Código original de E.D-OS:

- src/entry.S: entrada x86_64, pila inicial de 16 KiB y salto a kernel_main.
- src/kernel.c: núcleo freestanding, inicialización de COM1 y mensaje de diagnóstico KERNEL_BOOT_OK.
- linker.ld: segmentos ELF64 en el espacio virtual higher-half exigido por el protocolo Limine.
- Makefile: compilación con Clang/LLD y objetivo iso.
- limine.conf: entrada de arranque del kernel.
- scripts/build-iso.sh: creación de una ISO híbrida BIOS/UEFI.
- .github/workflows/kernel-build.yml: compilación y prueba de arranque en QEMU.

## Cargador y protocolo

Se ha seleccionado **Limine v12.9.6** como cargador externo y el protocolo Limine para el primer prototipo x86_64. Limine no es el kernel de E.D-OS: solo prepara el entorno de CPU/memoria y transfiere el control al ELF de nuestro kernel.

La release de Limine se descarga de forma fijada y se verifica con SHA-256 en CI. Limine se distribuye bajo BSD-2-Clause; se conservan sus avisos al incluir sus binarios en la imagen de prueba. Consultar el proyecto y sus instrucciones oficiales antes de actualizar la versión.

El kernel solicita la revisión base 3 del protocolo. Su entrada presupone el estado x86_64 long mode y usa una pila propia antes de llamar a C.

## Construcción

Requisitos: Clang, LLD, Make, binutils, xorriso, QEMU y una release binaria de Limine.

Compilar ELF:
```sh
make -C kernel clean all
```

Crear ISO híbrida (después de extraer Limine y definir LIMINE_DIR):
```sh
make -C kernel iso LIMINE_DIR=/ruta/a/limine
```

El resultado esperado es kernel/build/ed-os.iso. La imagen incluye arranque BIOS y UEFI x86_64.

## Prueba automatizada

GitHub Actions crea el ELF y la ISO, inicia QEMU por BIOS y comprueba que el puerto serie contiene KERNEL_BOOT_OK. Si QEMU se queda ejecutando el bucle halt del kernel, el proceso se detiene por timeout y se valida el mensaje de diagnóstico.

**Estado de la prueba QEMU: pendiente de resultado de GitHub Actions.** No se afirma todavía que el kernel haya arrancado con éxito.

## Límites actuales

- Solo objetivo x86_64; no hay soporte móvil/ARM64.
- Limine es una dependencia externa de arranque del prototipo; no se ha escrito un bootloader propio.
- No hay todavía gestor de memoria física/virtual, IDT, interrupciones activas, planificador, procesos, syscalls, drivers ni sistema de archivos.
- La prueba prevista es en QEMU; no demuestra compatibilidad con hardware físico.
- La compilación de un ELF por sí sola no demuestra que pueda arrancar.

## Reglas de trabajo

1. Mantener el código original de E.D-OS separado del código de terceros.
2. Estudiar dependencias e interfaces de cada subsistema Linux antes de considerar su adaptación.
3. Registrar URL, versión/commit, licencia, avisos, modificaciones y pruebas de cada componente externo.
4. No copiar código GPL-2.0-only sin revisar las obligaciones legales y de redistribución aplicables.
5. No declarar el kernel funcional hasta compilarlo y arrancarlo en un entorno controlado.
