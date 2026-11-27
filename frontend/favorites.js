(() => {
  const grid = document.querySelector('#favorites-grid');
  const count = document.querySelector('#favorites-count');
  const message = document.querySelector('#favorites-message');

  let favorites = [];

  // Cria o cartao de uma receita favorita
  function createFavoriteCard(recipe) {
    const card = document.createElement('article');
    card.className = 'recipe-card favorite-card';

    const content = document.createElement('div');
    content.className = 'recipe-content';

    const title = document.createElement('h3');
    title.textContent = recipe.name;

    if (recipe.available) {
      const link = document.createElement('a');
      link.className = 'recipe-card-link';
      link.href = `recipe.html?id=${encodeURIComponent(recipe.id)}&source=discover`;

      const image = document.createElement('div');
      image.className = 'recipe-image';
      image.textContent = recipe.name;

      if (typeof recipe.image_url === 'string' && recipe.image_url.startsWith('https://')) {
        const photo = document.createElement('img');
        photo.className = 'recipe-card-photo';
        photo.alt = '';
        photo.loading = 'lazy';
        photo.referrerPolicy = 'no-referrer';

        photo.addEventListener('load', () => {
          photo.classList.add('is-loaded');
        });

        photo.addEventListener('error', () => photo.remove());

        image.append(photo);
        photo.src = recipe.image_url;
      }

      const author = document.createElement('p');
      author.className = 'recipe-author';
      author.textContent = `Shared by ${recipe.author}`;

      const ingredients = document.createElement('p');
      const ingredientText = recipe.ingredient_list || '';

      ingredients.textContent =
        ingredientText.length > 140 ? ingredientText.slice(0, 140) + '…' : ingredientText;

      content.append(title, author, ingredients);
      link.append(image, content);
      card.append(link);
    } else {
      const notice = document.createElement('p');
      notice.textContent = 'This recipe is no longer publicly available.';

      content.append(title, notice);
      card.append(content);
    }

    const actions = document.createElement('div');
    actions.className = 'favorite-card-actions';

    const removeButton = document.createElement('button');
    removeButton.type = 'button';
    removeButton.className = 'remove-favorite-button';
    removeButton.textContent = '♥ Remove favorite';

    removeButton.addEventListener('click', async () => {
      removeButton.disabled = true;

      try {
        const response = await fetch(`/api/favorites/${encodeURIComponent(recipe.id)}`, {
          method: 'DELETE',
          credentials: 'same-origin',
        });

        if (response.status === 401) {
          window.location.replace('index.html');
          return;
        }

        const body = await response.json();

        if (!response.ok) {
          message.textContent = body.error || 'Unable to remove favorite.';
          return;
        }

        favorites = favorites.filter((item) => String(item.id) !== String(recipe.id));

        card.remove();
        updateCount();

        if (favorites.length === 0) {
          message.textContent = 'Your favorites collection is empty.';
        } else {
          message.textContent = 'Recipe removed from favorites.';
        }
      } catch {
        message.textContent = 'Could not connect to the server. Please try again.';
      } finally {
        removeButton.disabled = false;
      }
    });

    actions.append(removeButton);
    card.append(actions);

    return card;
  }

  function updateCount() {
    count.textContent = `${favorites.length} ${favorites.length === 1 ? 'favorite' : 'favorites'}`;
  }

  function displayFavorites() {
    grid.replaceChildren();
    updateCount();

    if (favorites.length === 0) {
      message.textContent = 'No favorites yet. Explore Discover to save recipes you love.';
      return;
    }

    message.textContent = '';

    favorites.forEach((recipe) => {
      grid.append(createFavoriteCard(recipe));
    });
  }

  // Carrega os favoritos do usuario
  async function loadFavorites() {
    try {
      const response = await fetch('/api/favorites', {
        credentials: 'same-origin',
        cache: 'no-store',
      });

      if (response.status === 401) {
        window.location.replace('index.html');
        return;
      }

      const body = await response.json();

      if (!response.ok || !Array.isArray(body.recipes)) {
        throw new Error('Unable to load favorites');
      }

      favorites = body.recipes;
      displayFavorites();
    } catch {
      message.textContent = 'Unable to load favorites. Please reload the page.';
    }
  }

  void loadFavorites();
})();
