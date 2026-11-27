const logoutButton = document.querySelector('#logout-button');
const recipeGrid = document.querySelector('#recipe-grid');
const recipeCount = document.querySelector('#recipe-count');
const collectionMessage = document.querySelector('#collection-message');
const searchInput = document.querySelector('#discover-search');

let allRecipes = [];

// Cria o cartao de uma receita publica
function createRecipeCard(recipe) {
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

  const details = document.createElement('span');
  details.className = 'recipe-time';

  const parts = [];

  if (recipe.servings != null) {
    parts.push(`${recipe.servings} servings`);
  }

  if (recipe.prep_minutes != null) {
    parts.push(`Prep: ${recipe.prep_minutes} min`);
  }

  if (recipe.cook_minutes != null) {
    parts.push(`Cook: ${recipe.cook_minutes} min`);
  }

  details.textContent = parts.join(' · ');

  content.append(title, author, ingredients, details);
  card.append(image, content);

  return card;
}

// Filtra as receitas pelo nome, ingredientes ou autor
function displayRecipes() {
  const search = searchInput.value.trim().toLowerCase();

  const recipes = allRecipes.filter((recipe) =>
    [recipe.name, recipe.ingredient_list, recipe.author].some((value) =>
      (value || '').toLowerCase().includes(search)
    )
  );

  recipeGrid.replaceChildren();

  recipeCount.textContent = search
    ? `${recipes.length} of ${allRecipes.length} recipes`
    : `${recipes.length} ${recipes.length === 1 ? 'recipe' : 'recipes'}`;

  if (recipes.length === 0) {
    collectionMessage.textContent = search
      ? 'No recipes match your search.'
      : 'No public recipes have been shared yet.';
    return;
  }

  collectionMessage.textContent = '';

  recipes.forEach((recipe) => {
    recipeGrid.append(createRecipeCard(recipe));
  });
}

// Carrega as receitas publicas do backend
async function loadRecipes() {
  try {
    const response = await fetch('/api/discover', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    const body = await response.json();

    if (!response.ok || !Array.isArray(body.recipes)) {
      collectionMessage.textContent = body.error || 'Unable to load community recipes.';
      return;
    }

    allRecipes = body.recipes;
    displayRecipes();
  } catch {
    collectionMessage.textContent = 'Unable to load community recipes. Please reload the page.';
  }
}

searchInput.addEventListener('input', displayRecipes);

logoutButton.addEventListener('click', async () => {
  logoutButton.disabled = true;

  try {
    const response = await fetch('/api/auth/logout', {
      method: 'POST',
      credentials: 'same-origin',
    });

    if (!response.ok) {
      throw new Error('Logout failed');
    }

    window.location.replace('index.html');
  } catch {
    alert('Could not log out. Please try again.');
    logoutButton.disabled = false;
  }
});

void loadRecipes();
