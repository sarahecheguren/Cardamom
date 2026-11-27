(() => {
  const grid = document.querySelector('#recommendations-grid');
  const message = document.querySelector('#recommendations-message');
  const shuffleButton = document.querySelector('#shuffle-recipes');

  let communityRecipes = [];

  // Embaralha as receitas sem modificar a lista original
  function shuffleRecipes(recipes) {
    const shuffled = [...recipes];

    for (let i = shuffled.length - 1; i > 0; i--) {
      const j = Math.floor(Math.random() * (i + 1));
      [shuffled[i], shuffled[j]] = [shuffled[j], shuffled[i]];
    }

    return shuffled;
  }

  // Cria o cartao de uma receita recomendada
  function createCard(recipe) {
    const card = document.createElement('a');
    card.className = 'recipe-card recipe-card-link';
    card.href = `recipe.html?id=${encodeURIComponent(recipe.id)}&source=discover`;

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

    const content = document.createElement('div');
    content.className = 'recipe-content';

    const title = document.createElement('h3');
    title.textContent = recipe.name;

    const author = document.createElement('p');
    author.className = 'recipe-author';
    author.textContent = `Shared by ${recipe.author}`;

    const ingredients = document.createElement('p');
    const ingredientText = recipe.ingredient_list || '';

    ingredients.textContent =
      ingredientText.length > 140 ? ingredientText.slice(0, 140) + '…' : ingredientText;

    content.append(title, author, ingredients);
    card.append(image, content);

    return card;
  }

  // Mostra ate tres receitas aleatorias
  function displayRecommendations() {
    grid.replaceChildren();

    if (communityRecipes.length === 0) {
      message.textContent = 'No community recommendations available yet. Check back later!';
      shuffleButton.disabled = true;
      return;
    }

    message.textContent = '';

    const recommendations = shuffleRecipes(communityRecipes).slice(0, 3);

    recommendations.forEach((recipe) => {
      grid.append(createCard(recipe));
    });

    shuffleButton.disabled = communityRecipes.length <= 1;
  }

  // Carrega as receitas publicas e exclui as do proprio usuario
  async function loadRecommendations() {
    try {
      const options = {
        credentials: 'same-origin',
        cache: 'no-store',
      };

      const [personalResponse, discoverResponse] = await Promise.all([
        fetch('/api/recipes', options),
        fetch('/api/discover', options),
      ]);

      if (personalResponse.status === 401 || discoverResponse.status === 401) {
        window.location.replace('index.html');
        return;
      }

      if (!personalResponse.ok || !discoverResponse.ok) {
        throw new Error('Unable to load recipes');
      }

      const personal = await personalResponse.json();
      const discover = await discoverResponse.json();

      if (!Array.isArray(personal.recipes) || !Array.isArray(discover.recipes)) {
        throw new Error('Invalid recipe response');
      }

      const ownedIds = new Set(personal.recipes.map((recipe) => String(recipe.id)));

      communityRecipes = discover.recipes.filter(
        (recipe) => recipe.visibility === 'public' && !ownedIds.has(String(recipe.id))
      );

      displayRecommendations();
    } catch {
      message.textContent = 'Unable to load recommendations. Please reload the page.';
    }
  }

  shuffleButton.addEventListener('click', displayRecommendations);

  void loadRecommendations();
})();
