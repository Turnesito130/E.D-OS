(() => {
  'use strict';

  const safeHttpUrl = value => {
    const raw = String(value || '').trim();
    if (!raw || raw.length > 2048) return null;
    try {
      const candidate = /^[a-z][a-z0-9+.-]*:\/\//i.test(raw) ? raw : 'https://' + raw;
      const url = new URL(candidate);
      if (!['https:', 'http:'].includes(url.protocol) || !url.hostname.includes('.')) return null;
      return url;
    } catch (_) { return null; }
  };

  const searchEngines = {
    duckduckgo: 'https://duckduckgo.com/?q=',
    google: 'https://www.google.com/search?q=',
    bing: 'https://www.bing.com/search?q='
  };

  function showMessage(message, linkUrl, linkLabel) {
    const messageBox = document.querySelector('[data-freedom-message]');
    if (!messageBox) return;
    messageBox.replaceChildren(document.createTextNode(message));
    if (linkUrl) {
      const link = document.createElement('a');
      link.href = linkUrl;
      link.target = '_blank';
      link.rel = 'noopener noreferrer';
      link.textContent = linkLabel || 'Abrir enlace';
      link.style.display = 'inline-block';
      link.style.marginLeft = '6px';
      messageBox.append(link);
    }
  }

  function openExternal(url, label) {
    // Called directly from a submit/click gesture. Never proxies the site or handles credentials.
    const opened = window.open(url, '_blank');
    if (opened) { try { opened.opener = null; } catch (_) {} }
    if (!opened) {
      showMessage('El navegador no abrió una pestaña nueva. Puedes usar este enlace:', url, label);
      return false;
    }
    showMessage('Se ha solicitado abrir el sitio en una pestaña nueva. Si el dispositivo o la red lo bloquean, E.D-OS no puede quitar esa restricción.');
    return true;
  }

  document.addEventListener('ed-os:window-opened', event => {
    const detail = event.detail || {};
    if (detail.name !== 'E.D. Freedom') return;
    const root = detail.element;
    const searchForm = root.querySelector('[data-freedom-search]');
    const openForm = root.querySelector('[data-freedom-download]');
    if (!searchForm || !openForm) return;

    searchForm.addEventListener('submit', submitEvent => {
      submitEvent.preventDefault();
      const input = searchForm.querySelector('input[name="q"]');
      const query = input ? input.value.trim() : '';
      const engineSelect = searchForm.querySelector('select[name="engine"]');
      const engine = engineSelect ? engineSelect.value : 'duckduckgo';
      if (!query) {
        showMessage('Escribe una búsqueda o una dirección web.');
        return;
      }

      // A domain typed into the search box is opened directly; ordinary text is searched.
      const directUrl = safeHttpUrl(query);
      const looksLikeAddress = /^(https?:\/\/|www\.)/i.test(query) || /^[a-z0-9-]+\.[a-z]{2,}([/:?#]|$)/i.test(query);
      if (looksLikeAddress && directUrl) {
        openExternal(directUrl.href, 'Abrir ' + directUrl.hostname);
        return;
      }

      const base = searchEngines[engine] || searchEngines.duckduckgo;
      openExternal(base + encodeURIComponent(query), 'Abrir resultados de búsqueda');
    });

    openForm.addEventListener('submit', submitEvent => {
      submitEvent.preventDefault();
      const input = openForm.querySelector('input[name="url"]');
      const url = safeHttpUrl(input ? input.value : '');
      if (!url) {
        showMessage('Dirección no válida. Escribe un dominio o una dirección HTTP/HTTPS.');
        return;
      }
      openExternal(url.href, 'Abrir ' + url.hostname);
    });

    root.querySelectorAll('[data-freedom-site]').forEach(button => {
      button.addEventListener('click', () => {
        const url = safeHttpUrl(button.dataset.freedomSite);
        if (url) openExternal(url.href, 'Abrir ' + url.hostname);
      });
    });
  });
})();