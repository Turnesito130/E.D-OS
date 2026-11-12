# E.D-OS — desarrollo del núcleo propio

E.D-OS tendrá un **kernel propio e independiente**. No utilizará el kernel Linux para arrancar ni para funcionar.

Linux se estudia como referencia de ingeniería. Cualquier reutilización de código será selectiva y requerirá revisar dependencias, procedencia, licencia y obligaciones de redistribución antes de incorporarlo.

## Prototipo de código 0.0.1

Ya existe una primera base de código original en kernel/src:

- src/entry.S: punto de entrada x86_64 y pila inicial de 16 KiB.
- src/kernel.c: núcleo freestanding mínimo y diagnóstico por puerto serie COM1.
- linker.ld: distribución ELF64 inicial a partir de 1 MiB.
- Makefile: compilación reproducible con Clang/LLD.
- .github/workflows/kernel-build.yml: compilación automatizada y publicación del ELF como artefacto.

El código no usa libc ni cabeceras del kernel Linux. El punto de entrada presupone que un cargador futuro transfiere el control en modo long 64-bit. **Todavía no se ha elegido ni implementado el protocolo/cargador de arranque**, por lo que este ELF no debe presentarse como un sistema arrancable.

## Objetivo inicial

- Arquitectura de desarrollo: x86_64.
- Primer entorno de ejecución previsto: QEMU.
- Primer hito de arranque: definir un protocolo de hand-off, preparar el cargador y demostrar que el kernel propio recibe el control en QEMU.
- Hardware físico y ARM64/móviles: objetivos posteriores, sujetos a drivers, firmware y pruebas reales.

## Estructura prevista

- research/linux/: notas y análisis; no contiene código copiado.
- design/: especificaciones de interfaces y arquitectura.
- src/: código fuente original del kernel E.D-OS.
- third_party/: componentes externos aprobados, con licencias y procedencia.
- patches/: adaptaciones mantenidas contra una versión concreta de un componente externo.
- manifest.json: estado y registro de fuentes externas.

## Reglas de trabajo

1. Mantener el código original de E.D-OS separado del código de terceros.
2. Estudiar las dependencias e interfaces de cada subsistema Linux antes de considerar su adaptación.
3. Registrar URL, versión/commit, licencia, avisos, modificaciones y pruebas de cada componente externo.
4. No copiar código GPL-2.0-only sin revisar las obligaciones legales y de redistribución aplicables.
5. Preferir una implementación propia cuando portar un subsistema Linux resulte demasiado acoplado o costoso.
6. No declarar el kernel funcional hasta compilarlo y arrancarlo en un entorno controlado.
7. No afirmar que una compilación ELF equivale a un arranque probado.

## Estado verificado

- [x] Código fuente mínimo de entrada y núcleo creado.
- [x] Compilación local de prueba con Clang/LLD: ELF64 x86-64 generado correctamente.
- [ ] Compilación automatizada de GitHub Actions confirmada.
- [ ] Protocolo/cargador de arranque seleccionado e implementado.
- [ ] Arranque del kernel propio probado en QEMU.
- [ ] Memoria física/virtual e interrupciones implementadas.

La prueba local solo verifica que el código compila y enlaza. No demuestra que el kernel arranque, que COM1 funcione en una máquina virtual ni que exista compatibilidad con hardware físico.
