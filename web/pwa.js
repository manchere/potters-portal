if ("serviceWorker" in navigator) {
  window.addEventListener("load", () => {
    navigator.serviceWorker.register("/service-worker.js").catch(() => {
      // Installability is a nice-to-have; a failed registration shouldn't
      // break the app.
    });
  });
}
