# Inventario de reutilización de componentes — E.D-OS

Fecha: 2026-11-13

## Objetivo

Acelerar E.D-OS reutilizando componentes de software libre cuando su licencia, arquitectura y dependencias lo permitan. No se copiará código propietario ni se incorporarán componentes sin revisar sus licencias y procedencia.

## Decisión de arquitectura

Se mantienen dos líneas separadas:

- `arquitectura/0.1`: kernel propio de E.D-OS. No incorporar código del kernel Linux directamente aquí sin una decisión explícita de licencia y arquitectura.
- `experimento/linux-base`: evaluar Linux como base temporal para una versión funcional temprana y para aprovechar su ecosistema de controladores y software.

El objetivo de independencia futura exige interfaces propias (HAL, servicios y APIs estables) y pruebas de sustitución. No existe un mecanismo automático para retirar Linux: sus drivers y subsistemas dependen de interfaces internas.

## Inventario inicial de 10 áreas

| Área | Proyecto a evaluar | Reutilización posible | Encaje inicial | Riesgo / trabajo pendiente |
|---|---|---|---|---|
| Arranque | Limine | Bootloader existente; ya está en el arranque del kernel propio | Alto, conservar inicialmente | Verificar versión, licencia y configuración exactas |
| Kernel y memoria | FreeBSD | Estudiar/adaptar algoritmos y módulos aislados | Medio-bajo para el kernel propio | Dependencias BSD internas; portar no es copiar y pegar |
| Kernel alternativo | seL4 | Microkernel completo o referencia de aislamiento/capacidades | Medio como investigación; bajo como injerto | Arquitectura muy distinta; kernel GPLv2 y pruebas formales delicadas |
| Drivers y compatibilidad | Linux | Reutilización directa dentro de la rama Linux | Alto en experimento Linux | GPL-2.0; acoplamiento fuerte al kernel Linux |
| Compilador | LLVM/Clang | Toolchain para compilar E.D-OS | Alto | Licencia Apache-2.0 con LLVM exceptions; revisar componentes y binarios |
| Gráficos | Mesa | Implementaciones OpenGL/Vulkan y drivers gráficos | Alto sobre Linux; bajo en kernel propio hoy | Requiere DRM/KMS, memoria y drivers compatibles; licencias por componente |
| Red | FreeBSD / Linux | Pila TCP/IP y drivers en su entorno original; estudiar algoritmos | Alto sobre Linux; medio-bajo en kernel propio | Integración y licencias; no trasladar subsistemas enteros a ciegas |
| Sistema de archivos | FreeBSD / Linux | Sistemas de archivos maduros y herramientas | Alto sobre Linux; medio en kernel propio | VFS, bloque, caché y dependencias del kernel |
| Servicios de usuario / utilidades | BusyBox | Utilidades básicas para entorno Linux experimental | Alto en Linux | GPL-2.0; no proporciona un kernel ni un escritorio |
| Dispositivos integrados | Zephyr | Drivers, protocolos y patrones modulares para hardware embebido | Bajo para escritorio x86-64; útil en dispositivos E.D. futuros | Diseñado para sistemas embebidos; Apache-2.0 con excepciones por componentes |

## Prioridad recomendada

### Grupo A — aprovechar ahora
1. **Limine**: mantener el bootloader existente y documentar su configuración/licencia.
2. **LLVM/Clang**: usar como herramientas de compilación, sin convertirlo en parte del kernel.
3. **Linux + BusyBox + Mesa**: evaluar en `experimento/linux-base` para lograr una plataforma funcional temprana y comprobar arranque, red y gráficos por fases.

### Grupo B — estudiar antes de portar
4. **FreeBSD**: estudiar componentes concretos de red, almacenamiento y memoria; elegir un módulo solo después de identificar dependencias y licencia fichero a fichero.
5. **seL4**: estudiar su diseño de capacidades y aislamiento. No intentar injertar su kernel dentro del kernel actual.

### Grupo C — para hardware futuro
6. **Zephyr**: valorar para firmware, sensores y dispositivos integrados del ecosistema E.D., no como sustituto directo de un sistema operativo de escritorio.

## Reglas obligatorias antes de incorporar código

- Identificar repositorio, versión y commit exactos.
- Revisar licencia del proyecto y del fichero concreto (SPDX), avisos de copyright, dependencias y excepciones.
- Registrar origen, cambios realizados y obligaciones de redistribución.
- No asumir que la licencia del proyecto cubre todos sus subcomponentes.
- Mantener componentes GPL en una arquitectura compatible y revisar las obligaciones de distribución con asesoramiento jurídico antes de publicar un producto.
- Compilar y probar cada integración en una rama aislada; no mezclar experimentos con `main`.
- Mantener una alternativa o interfaz propia cuando la independencia futura sea un requisito.

## Próximo paso técnico

En la rama `experimento/linux-base`, verificar el resultado real del workflow de arranque y después comprobar, por separado, consola, almacenamiento, red y gráficos. En `arquitectura/0.1`, continuar el gestor de memoria propio y definir interfaces, sin importar código Linux directamente.

## Fuentes oficiales consultadas

- Linux kernel licensing rules: https://www.kernel.org/doc/html/latest/process/license-rules.html
- FreeBSD license guide: https://docs.freebsd.org/en/articles/license-guide/
- seL4 licensing: https://sel4.org/Legal/license.html
- Zephyr introduction and licensing: https://docs.zephyrproject.org/latest/introduction/index.html
- Zephyr component licensing: https://docs.zephyrproject.org/latest/LICENSING.html
- Mesa license and copyright: https://docs.mesa3d.org/license.html

Este documento es un inventario preliminar, no una autorización jurídica ni una afirmación de que los componentes ya estén integrados o probados.
