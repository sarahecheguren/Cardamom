const recipeForm = document.querySelector('#recipe-form');

// Verifica se o usuario esta logado antes de acessar a pagina
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

// Impede o envio enquanto o backend de receitas nao estiver pronto
recipeForm.addEventListener('submit', (event) => {
  event.preventDefault();
});

// Verifica a sessao quando a pagina abre
checkSession();
