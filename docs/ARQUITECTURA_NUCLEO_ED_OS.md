# E.D-OS — Arquitectura del núcleo propio (borrador 0.2)

## Objetivo y decisión

E.D-OS tendrá un **núcleo propio**. No arrancará ni dependerá del kernel Linux para funcionar.

Estudiaremos el código fuente de Linux para comprender soluciones probadas, comparar diseños y localizar componentes que puedan reutilizarse o adaptarse legal y técnicamente. Linux será una referencia de ingeniería y una posible fuente de componentes seleccionados; no será el kernel de E.D-OS ni su base de ejecución.

Esto no significa que podamos copiar subsistemas completos y pegarlos sin más: el kernel Linux está fuertemente integrado, y cada pieza requiere estudiar sus dependencias, interfaces, arquitectura y licencia. Los fragmentos derivados del código Linux mantienen sus obligaciones de licencia, normalmente GPL-2.0-only. La procedencia y las obligaciones se documentarán antes de incorporar código.

## Principios

- **Independencia de arranque:** el arranque normal de E.D-OS no requerirá un kernel Linux.
- **Diseño propio:** definir interfaces, estructuras de datos y límites entre subsistemas de E.D-OS.
- **Estudio antes de copiar:** analizar subsistemas Linux, documentar qué problema resuelven y decidir si conviene implementar una solución propia o adaptar código.
- **Reutilización selectiva:** incorporar código solo cuando sea técnicamente viable, compatible con la arquitectura y con licencia revisada.
- **Trazabilidad:** registrar origen, autores, licencia, modificaciones y dependencias de cada componente incorporado.
- **Pruebas reproducibles:** no afirmar que el núcleo funciona hasta compilarlo y probarlo en un entorno controlado.

## Arquitectura objetivo (propuesta inicial)

1. **Firmware y arranque:** UEFI, cargador propio o seleccionado, verificación de la cadena de arranque.
2. **Núcleo E.D-OS:** interrupciones, gestión de memoria física y virtual, planificación de tareas, llamadas al sistema, IPC y temporizadores.
3. **Controladores:** modelo de drivers y control de acceso a dispositivos; soporte por hardware añadido gradualmente.
4. **Servicios fundamentales:** gestor de procesos, sistema de archivos, red, registro de eventos y configuración.
5. **Seguridad:** aislamiento, permisos, capacidades, gestión de claves, actualizaciones verificadas y recuperación.
6. **Plataforma:** runtime de aplicaciones, servicios del sistema, interfaz y accesibilidad.
7. **Ecosistema:** E.D. Connect, cuentas, sincronización y E.D. Cloud. La nube no será requisito para arrancar ni para usar el equipo.
8. **E.D.:** IA integrada con permisos explícitos; sin acceso implícito a todos los datos o dispositivos.

La arquitectura es una propuesta, no una implementación existente. Los límites entre subsistemas se concretarán mediante especificaciones y prototipos.

## Plan de trabajo

### N0 — Investigación y diseño
- [ ] Estudiar la estructura general del kernel Linux y sus subsistemas.
- [ ] Crear una matriz de subsistemas: función, interfaces, dependencias, complejidad y licencia.
- [ ] Definir requisitos y objetivos de hardware iniciales.
- [ ] Especificar el modelo de arranque, memoria, interrupciones y llamadas al sistema de E.D-OS.
- [ ] Elegir lenguaje, toolchain y entorno reproducible.

### N1 — Núcleo mínimo independiente
- [ ] Crear un proyecto de kernel E.D-OS separado del código Linux.
- [ ] Arrancar un núcleo mínimo propio en QEMU x86_64/UEFI.
- [ ] Implementar salida de diagnóstico, gestión básica de memoria e interrupciones.
- [ ] Documentar logs y pasos para reproducir cada prueba.

### N2 — Subsistemas esenciales
- [ ] Diseñar e implementar planificación de tareas y cambios de contexto.
- [ ] Diseñar memoria virtual y aislamiento entre procesos.
- [ ] Definir llamadas al sistema e IPC.
- [ ] Crear pruebas unitarias y pruebas de integración en QEMU.

### N3 — Reutilización evaluada
- [ ] Evaluar componentes concretos de Linux u otros proyectos por separado.
- [ ] Revisar licencia, dependencias, acoplamiento y coste de mantenimiento de cada candidato.
- [ ] Adaptar únicamente componentes aprobados, conservando avisos y trazabilidad.
- [ ] Mantener una separación clara entre código original E.D-OS y código derivado de terceros.

### N4 — Plataforma arrancable
- [ ] Añadir drivers y sistema de archivos iniciales.
- [ ] Crear servicios mínimos y un entorno de usuario.
- [ ] Probar instalación, actualización y recuperación en máquina virtual.
- [ ] Validar cada modelo de hardware antes de anunciar compatibilidad.

## Qué significa estudiar Linux

Podemos usar Linux para aprender cómo resuelve problemas reales y analizar su código. También podemos compilarlo en una máquina virtual de laboratorio si eso ayuda a observar su comportamiento; esa compilación sería solo una herramienta de estudio y **no formaría parte del arranque de E.D-OS**.

No se copiará un subsistema sin entender sus dependencias. Un componente que parezca aislado puede depender de tipos, APIs, infraestructura de memoria, sincronización o servicios internos del kernel Linux. En algunos casos será más seguro y mantenible implementar una solución propia inspirada en el comportamiento documentado.

## Licencias y procedencia

El kernel Linux se distribuye principalmente bajo GPL-2.0-only, con excepciones y archivos que pueden tener condiciones específicas. Antes de incorporar cualquier código se comprobarán los avisos SPDX, la licencia exacta, los derechos de autor, las dependencias y las obligaciones de redistribución. No se asumirá que todo el árbol tiene una única licencia ni que estudiar código obliga a copiarlo.

## Estado real

Por ahora solo existe documentación de arquitectura y planificación. No se ha creado, compilado ni arrancado un kernel propio de E.D-OS. Tampoco se ha importado código Linux. Los siguientes hitos deben producir código y pruebas verificables, no solo documentos.

## Criterio de finalización

Una fase solo se marca como terminada cuando hay código, instrucciones reproducibles, versiones, configuración, logs y resultados de pruebas. Un commit o un documento no demuestra por sí solo que el núcleo compile o arranque.
