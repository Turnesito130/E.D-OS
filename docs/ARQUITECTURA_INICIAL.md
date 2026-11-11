# E.D-OS — Arquitectura inicial 0.1

**Estado:** fase 0 — diseño y decisiones de arquitectura  
**Rama de trabajo:** `arquitectura/0.1`  
**Proyecto:** E.D-OS es independiente del proyecto de IA personal E.D. (repositorio `e.d-2`).

## 1. Visión

E.D-OS aspira a ser un sistema operativo y ecosistema tecnológico propio, coherente, seguro, accesible y reconocible. No se considera terminado ni funcional todavía: este documento fija la primera arquitectura que habrá que validar mediante prototipos.

## 2. Principios de diseño

1. **Experiencia coherente:** interfaz, ajustes y aplicaciones deben sentirse parte del mismo sistema.
2. **Seguridad desde el diseño:** permisos explícitos, aislamiento entre aplicaciones y actualizaciones verificables.
3. **Control del usuario:** transparencia sobre datos, procesos, conexiones y permisos.
4. **Rendimiento:** funcionar de forma fluida también en hardware modesto cuando sea posible.
5. **Compatibilidad gradual:** priorizar primero una plataforma de pruebas realista antes de intentar soportar muchos dispositivos.
6. **Honestidad técnica:** distinguir siempre entre diseño, prototipo, versión ejecutable y sistema operativo instalable.

## 3. Separación de proyectos

- **E.D. personal:** asistente privado del usuario, desarrollado en `e.d-2`.
- **E.D-OS:** sistema operativo y experiencia de producto de este repositorio.
- **IA comercial futura:** proyecto distinto, con objetivos, datos y permisos propios.

No se compartirán automáticamente memorias, credenciales ni datos entre estos proyectos.

## 4. Arquitectura por capas

| Capa | Responsabilidad | Primer objetivo |
|---|---|---|
| Experiencia e interfaz | Escritorio, ventanas, navegación, accesibilidad y ajustes | Definir identidad visual y flujo principal |
| Aplicaciones | Aplicaciones del sistema y formato de paquetes | Especificar una aplicación de ejemplo |
| Servicios del sistema | Sesiones, notificaciones, red, archivos y actualizaciones | Dibujar interfaces entre servicios |
| Seguridad | Usuarios, permisos, aislamiento y políticas | Modelo de permisos mínimo |
| Núcleo y hardware | Procesos, memoria, almacenamiento y controladores | Evaluar una base técnica viable |

## 5. Estrategia técnica provisional

La primera meta no será escribir un kernel desde cero. Primero construiremos y probaremos la experiencia y los componentes propios sobre una base técnica existente y compatible con el hardware de pruebas. Una base Linux es una opción inicial razonable, pero todavía no se ha elegido una distribución ni se ha tomado una decisión irreversible sobre el kernel.

Un kernel propio sería una etapa avanzada, separada y condicionada a disponer de conocimientos, pruebas y recursos suficientes.

## 6. Alcance de la versión 0.1

La versión 0.1 es una **fase de arquitectura**, no una ISO ni un sistema instalable. Sus entregables serán:

- mapa de componentes y sus límites;
- decisiones técnicas justificadas;
- requisitos mínimos de hardware y plataforma de pruebas;
- diseño inicial de interfaz;
- plan de prototipo y pruebas.

## 7. Próximos pasos

1. Validar esta arquitectura y corregir lo que no represente la visión del proyecto.
2. Elegir la plataforma de pruebas según el hardware realmente disponible.
3. Diseñar el escritorio y la navegación principales.
4. Construir un prototipo pequeño y comprobarlo antes de ampliar el alcance.

## 8. Registro de decisiones pendientes

- Base técnica concreta y distribución: pendiente.
- Entorno gráfico y toolkit: pendiente.
- Modelo de aplicaciones y paquetes: pendiente.
- Estrategia de actualizaciones: pendiente.
- Hardware objetivo inicial: pendiente de pruebas.

---

**Regla del proyecto:** no afirmar que una función existe hasta que esté implementada y probada.
