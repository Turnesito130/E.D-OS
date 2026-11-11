(() => {
  'use strict';
  const layer = document.getElementById('windowLayer');
  if (!layer) return;
  let topZ = 20;
  const windows = new Map();
  const definitions = {
    'Inicio': '<p class="window-note">Bienvenido a E.D-OS. Este es el espacio de inicio del escritorio.</p><p class="window-note">Abre otras aplicaciones desde la barra inferior. Puedes mantener varias ventanas abiertas a la vez.</p>',
    'Archivos': '<p class="window-note">Organiza documentos dentro del almacenamiento local de este navegador.</p><p><a href="./apps/archivos.html">Abrir Archivos →</a></p><p class="window-note">Todavía no accede al disco real ni sincroniza archivos.</p>',
    'Navegador': '<p class="window-note">El navegador integrado todavía no está implementado.</p><p class="window-note">Por ahora puedes abrir un navegador externo desde tu dispositivo.</p>',
    'Ajustes': '<p class="window-note">Personaliza el tema, el tamaño del texto y las preferencias de interacción.</p><p><a href="./apps/ajustes.html">Abrir Ajustes →</a></p>',
    'E.D. Connect': '<p class="window-note">Prueba experimental de conexión directa entre navegadores.</p><p><a href="./connect.html">Abrir E.D. Connect →</a></p><p class="window-note">Todavía no ofrece emparejamiento automático ni transferencia de archivos.</p>',
    'Buscar': '<label class="search-label" for="edSearchInput">Buscar en E.D-OS</label><input class="system-search-input" id="edSearchInput" data-search-input type="search" placeholder="Aplicaciones, tareas y archivos…" autocomplete="off"><div data-search-results></div>',
    'Notificaciones': '<div class="notification-toolbar"><p class="window-note">Avisos recientes de este navegador.</p><button type="button" class="window-control" data-clear-notifications>Limpiar</button></div><div data-notification-list></div>'
  };
  function focus(win) { topZ += 1; win.style.zIndex = String(topZ); }
  function close(name) {
    const win = windows.get(name);
    if (win) { win.remove(); windows.delete(name); }
  }
  function makeDraggable(win, bar) {
    let drag = null;
    bar.addEventListener('pointerdown', event => {
      if (event.target.closest('button')) return;
      if (win.classList.contains('maximized')) return;
      focus(win);
      const rect = win.getBoundingClientRect();
      drag = { x: event.clientX, y: event.clientY, left: rect.left, top: rect.top };
      bar.setPointerCapture(event.pointerId);
    });
    bar.addEventListener('pointermove', event => {
      if (!drag) return;
      const left = Math.max(0, Math.min(window.innerWidth - 140, drag.left + event.clientX - drag.x));
      const top = Math.max(0, Math.min(window.innerHeight - 90, drag.top + event.clientY - drag.y));
      win.style.left = left + 'px';
      win.style.top = top + 'px';
      win.style.transform = 'none';
    });
    const stop = () => { drag = null; };
    bar.addEventListener('pointerup', stop);
    bar.addEventListener('pointercancel', stop);
  }
  function open(name) {
    let win = windows.get(name);
    if (win) {
      win.hidden = false;
      win.classList.remove('minimized');
      focus(win);
      return;
    }
    win = document.createElement('section');
    win.className = 'window open managed-window';
    win.dataset.appWindow = name;
    win.setAttribute('role', 'dialog');
    win.setAttribute('aria-label', name);
    win.style.zIndex = String(++topZ);
    win.innerHTML = '<header class="window-head"><h2></h2><div class="window-controls"><button type="button" class="window-control minimize" aria-label="Minimizar" title="Minimizar">−</button><button type="button" class="window-control maximize" aria-label="Maximizar" title="Maximizar">□</button><button type="button" class="window-control close" aria-label="Cerrar" title="Cerrar">×</button></div></header><div class="window-content"></div>';
    win.querySelector('h2').textContent = name;
    win.querySelector('.window-content').innerHTML = definitions[name] || '<p class="window-note">Esta aplicación todavía no está implementada.</p>';
    layer.append(win);
    windows.set(name, win);
    makeDraggable(win, win.querySelector('.window-head'));
    win.addEventListener('pointerdown', () => focus(win));
    win.querySelector('.minimize').addEventListener('click', () => { win.hidden = true; win.classList.add('minimized'); });
    win.querySelector('.maximize').addEventListener('click', event => {
      const maximized = win.classList.toggle('maximized');
      event.currentTarget.textContent = maximized ? '❐' : '□';
      event.currentTarget.title = maximized ? 'Restaurar' : 'Maximizar';
      event.currentTarget.setAttribute('aria-label', event.currentTarget.title);
      if (!maximized) { win.style.left = ''; win.style.top = ''; win.style.width = ''; win.style.height = ''; win.style.transform = ''; }
    });
    win.querySelector('.close').addEventListener('click', () => close(name));
    if (window.innerWidth < 620) win.classList.add('mobile-window');
  }
  document.addEventListener('ed-os:open-app', event => { if (event.detail && typeof event.detail.name === 'string') open(event.detail.name); });\n  document.querySelectorAll('[data-app]').forEach(button => {
    button.addEventListener('click', () => open(button.dataset.app));
  });
  document.getElementById('ed')?.addEventListener('click', () => open('Asistente E.D.'));
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape') {
      const active = [...windows.values()].sort((a,b) => Number(b.style.zIndex) - Number(a.style.zIndex))[0];
      if (active) close(active.dataset.appWindow);
    }
  });
})();
