(() => {
  'use strict';
  const layer = document.getElementById('windowLayer');
  if (!layer) return;
  let topZ = 20;
  const windows = new Map();

  const frame = (name, src) => '<iframe class="app-frame" title="' + name + '" src="' + src + '"></iframe>';
  const definitions = {
    'Inicio': '<p class="window-note">Escritorio principal. Abre las aplicaciones desde el dock.</p><p class="window-note">Puedes mantener varias ventanas abiertas, moverlas, minimizarlas y maximizarlas.</p>',
    'Archivos': frame('Archivos', './apps/archivos.html'),
    'Ajustes': frame('Ajustes', './apps/ajustes.html'),
    'E.D. Connect': frame('E.D. Connect', './connect.html'),
    'Navegador': '<p class="window-note">Busca en Internet o abre una dirección. Las páginas se abrirán en una pestaña del navegador de tu dispositivo; esta aplicación no intenta imitar ni sustituir Chrome.</p><form data-browser-form class="freedom-form"><label>Buscar o escribir una dirección</label><input name="query" type="search" required maxlength="2048" placeholder="Ej.: TikTok, Wikipedia o ejemplo.com"><label>Buscador</label><select name="engine"><option value="https://duckduckgo.com/?q=">DuckDuckGo</option><option value="https://www.google.com/search?q=">Google</option><option value="https://www.bing.com/search?q=">Bing</option></select><button type="submit">Buscar / abrir</button></form><div class="freedom-shortcuts"><span class="window-note">Accesos rápidos</span><button type="button" data-browser-url="https://www.tiktok.com/">TikTok</button><button type="button" data-browser-url="https://www.youtube.com/">YouTube</button><button type="button" data-browser-url="https://www.instagram.com/">Instagram</button><button type="button" data-browser-url="https://www.wikipedia.org/">Wikipedia</button></div><p class="window-note" data-browser-status aria-live="polite">Los inicios de sesión se realizan únicamente en la web oficial.</p>',
    'E.D. Freedom': '<p class="window-note">Busca en Internet o abre directamente una dirección. Los sitios se abren en una pestaña del navegador; el inicio de sesión lo realiza el propio sitio.</p><form data-freedom-search class="freedom-form"><label>Buscar o escribir una dirección</label><input name="q" type="search" required maxlength="240" placeholder="Ej.: TikTok, noticias o tiktok.com"><label>Buscador para búsquedas de texto</label><select name="engine"><option value="duckduckgo">DuckDuckGo</option><option value="google">Google</option><option value="bing">Bing</option></select><button type="submit">Buscar / abrir</button></form><div class="freedom-shortcuts"><span class="window-note">Accesos directos</span><button type="button" data-freedom-site="https://www.tiktok.com/">TikTok</button><button type="button" data-freedom-site="https://www.youtube.com/">YouTube</button><button type="button" data-freedom-site="https://www.instagram.com/">Instagram</button><button type="button" data-freedom-site="https://www.wikipedia.org/">Wikipedia</button></div><form data-freedom-download class="freedom-form"><label>Abrir un enlace</label><input name="url" type="text" required maxlength="2048" placeholder="https://sitio.com"><button type="submit">Abrir sitio / archivo</button></form><p class="window-note">Escribe tu contraseña únicamente en la página oficial. E.D-OS nunca te la pedirá ni puede retirar bloqueos impuestos por el navegador, la red o el administrador del dispositivo.</p><p class="window-note" data-freedom-message aria-live="polite">Listo para abrir sitios y búsquedas.</p>',
    'Asistente E.D.': '<p class="window-note"><strong>El asistente E.D. aún no está conectado.</strong></p><p class="window-note">No voy a simular una inteligencia artificial ni afirmar que razona cuando todavía no existe un motor propio integrado. Esta ventana queda preparada para una futura integración.</p>',
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
      if (event.target.closest('button') || win.classList.contains('maximized')) return;
      focus(win);
      const rect = win.getBoundingClientRect();
      drag = { x: event.clientX, y: event.clientY, left: rect.left, top: rect.top };
      bar.setPointerCapture(event.pointerId);
    });
    bar.addEventListener('pointermove', event => {
      if (!drag) return;
      const left = Math.max(0, Math.min(window.innerWidth - 140, drag.left + event.clientX - drag.x));
      const top = Math.max(0, Math.min(window.innerHeight - 90, drag.top + event.clientY - drag.y));
      win.style.left = left + 'px'; win.style.top = top + 'px'; win.style.transform = 'none';
    });
    const stop = () => { drag = null; };
    bar.addEventListener('pointerup', stop);
    bar.addEventListener('pointercancel', stop);
  }

  function bindBrowser(root) {
    const form = root.querySelector('[data-browser-form]');
    const status = root.querySelector('[data-browser-status]');
    if (!form || !status) return;
    const launch = (value) => {
      const raw = String(value || '').trim();
      if (!raw) { status.textContent = 'Escribe una búsqueda o una dirección.'; return; }
      let url;
      try {
        const candidate = /^[a-z][a-z0-9+.-]*:\/\//i.test(raw) ? raw : 'https://' + raw;
        const parsed = new URL(candidate);
        if (['http:', 'https:'].includes(parsed.protocol) && parsed.hostname.includes('.') && /^(https?:\/\/|www\.|[a-z0-9-]+\.[a-z]{2,})/i.test(raw)) url = parsed.href;
      } catch (_) {}
      if (!url && /^[a-z][a-z0-9+.-]*:\/\//i.test(raw)) { status.textContent = 'Solo se permiten direcciones HTTP o HTTPS.'; return; }
      if (!url) {
        const engine = form.querySelector('[name="engine"]').value;
        url = engine + encodeURIComponent(raw);
      }
      const opened = window.open(url, '_blank');
      if (opened) { try { opened.opener = null; } catch (_) {} }
      status.textContent = opened ? 'Se ha solicitado abrir la página en otra pestaña.' : 'El navegador bloqueó la ventana. Permite las ventanas emergentes o copia la dirección: ' + url;
    };
    form.addEventListener('submit', event => { event.preventDefault(); launch(form.querySelector('[name="query"]').value); });
    root.querySelectorAll('[data-browser-url]').forEach(button => button.addEventListener('click', () => launch(button.dataset.browserUrl)));
  }

  function open(name) {
    let win = windows.get(name);
    if (win) { win.hidden = false; win.classList.remove('minimized'); focus(win); return; }
    win = document.createElement('section');
    win.className = 'window open managed-window';
    win.dataset.appWindow = name;
    win.setAttribute('role', 'dialog');
    win.setAttribute('aria-label', name);
    win.style.zIndex = String(++topZ);
    win.innerHTML = '<header class="window-head"><h2></h2><div class="window-controls"><button type="button" class="window-control minimize" aria-label="Minimizar" title="Minimizar">−</button><button type="button" class="window-control maximize" aria-label="Maximizar" title="Maximizar">□</button><button type="button" class="window-control close" aria-label="Cerrar" title="Cerrar">×</button></div></header><div class="window-content"></div>';
    win.querySelector('h2').textContent = name;
    win.querySelector('.window-content').innerHTML = definitions[name] || '<p class="window-note">Esta aplicación todavía no está implementada.</p>';
    layer.append(win); windows.set(name, win);
    document.dispatchEvent(new CustomEvent('ed-os:window-opened', { detail: { name, element: win } }));
    if (name === 'Navegador') bindBrowser(win);
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

  document.addEventListener('ed-os:open-app', event => { if (event.detail && typeof event.detail.name === 'string') open(event.detail.name); });
  document.querySelectorAll('[data-app]').forEach(button => button.addEventListener('click', () => open(button.dataset.app)));
  document.getElementById('ed')?.addEventListener('click', () => open('Asistente E.D.'));
  document.addEventListener('keydown', event => {
    if (event.key === 'Escape') {
      const active = [...windows.values()].filter(win => !win.hidden).sort((a,b) => Number(b.style.zIndex) - Number(a.style.zIndex))[0];
      if (active) close(active.dataset.appWindow);
    }
  });
})();
