# E.D-OS — espacio de desarrollo del núcleo propio

E.D-OS está destinado a tener un **kernel propio e independiente**. El sistema no utilizará el kernel Linux para arrancar ni para funcionar.

Este directorio organiza el estudio técnico de Linux y el desarrollo del núcleo E.D-OS. El código Linux puede servir como referencia y, tras una evaluación individual, como origen de componentes seleccionados. No se importará el árbol completo ni se asumirá que sus subsistemas se pueden trasladar directamente.

## Objetivo inicial

- Primer entorno de desarrollo y pruebas: x86_64 en QEMU con firmware UEFI/OVMF.
- Primer hito: arrancar un kernel mínimo escrito para E.D-OS, independiente de Linux.
- Hardware físico y ARM64/móviles: objetivos posteriores, sujetos a drivers, firmware y pruebas reales.

## Reglas de trabajo

1. Mantener el código original de E.D-OS separado del código de terceros.
2. Estudiar las dependencias e interfaces de cada subsistema Linux antes de considerar su adaptación.
3. Registrar para cada componente externo su URL de origen, versión o commit, licencia, avisos, modificaciones y pruebas.
4. No copiar código GPL-2.0-only sin revisar las obligaciones legales y de redistribución aplicables.
5. Preferir una implementación propia cuando portar un subsistema Linux resulte demasiado acoplado o costoso.
6. No declarar que E.D-OS tiene un kernel funcional hasta que el kernel propio compile y arranque en QEMU.

## Estructura prevista

- `research/linux/`: notas y análisis de subsistemas; no contiene código copiado.
- `design/`: especificaciones de interfaces y arquitectura de E.D-OS.
- `src/`: código fuente original del kernel E.D-OS.
- `third_party/`: componentes externos aprobados, con sus licencias y procedencia.
- `patches/`: adaptaciones mantenidas contra una versión concreta de un componente externo, cuando proceda.
- `manifest.json`: estado de investigación y registro de fuentes externas.

## Primeros hitos

- [ ] Documentar la arquitectura general del kernel Linux.
- [ ] Analizar memoria, planificación, interrupciones, IPC y drivers.
- [ ] Definir la arquitectura mínima del kernel E.D-OS.
- [ ] Crear el proyecto de kernel propio y su toolchain reproducible.
- [ ] Arrancar el primer kernel E.D-OS en QEMU.
- [ ] Evaluar por separado cualquier código externo candidato.

## Límite actual

El escritorio web y la API del ecosistema no son un sistema operativo arrancable. Este directorio contiene planificación: todavía no hay kernel propio, bootloader, root filesystem ni pruebas de arranque.
