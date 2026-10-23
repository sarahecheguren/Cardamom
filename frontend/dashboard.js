const logoutButton = document.querySelector('#logout-button');

// Verifica se o usuario esta logado
async function checkSession() {
  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (!response.ok) {
      window.location.replace('index.html');
    }
  } catch (error) {
    window.location.replace('index.html');
  }
}

// Faz logout e volta para a pagina de login
logoutButton.addEventListener('click', async () => {
  logoutButton.disabled = true;

  try {
    const response = await fetch('/api/auth/logout', {
      method: 'POST',
      credentials: 'same-origin',
    });

    if (!response.ok) {
      alert('Could not log out. Please try again.');
      logoutButton.disabled = false;
      return;
    }

    window.location.replace('index.html');
  } catch (error) {
    alert('Cannot reach the server. Please try again.');
    logoutButton.disabled = false;
  }
});

// Verifica a sessao quando a pagina abre
checkSession();
