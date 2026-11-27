const logoutButton = document.querySelector('#logout-button');
const recipeGrid = document.querySelector('#recipe-grid');
const recipeCount = document.querySelector('#recipe-count');
const collectionMessage = document.querySelector('#collection-message');
const searchInput = document.querySelector('#recipe-search');

let allRecipes = [];

function displayRecipes(recipes) {
  recipeGrid.replaceChildren();
  recipeCount.textContent =
    recipes.length === allRecipes.length
      ? `${recipes.length} ${recipes.length === 1 ? 'recipe' : 'recipes'}`
      : `${recipes.length} of ${allRecipes.length} recipes`;

  if (recipes.length === 0) {
    collectionMessage.textContent =
      allRecipes.length === 0
        ? 'Your collection is empty. Use + Add Recipe to get started.'
        : 'No recipes match your search.';
    return;
  }

  collectionMessage.textContent = '';

  recipes.forEach((recipe) => {
    const card = document.createElement('a');
    card.className = 'recipe-card recipe-card-link';
    card.href = `recipe.html?id=${encodeURIComponent(recipe.id)}`;

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

    const ingredients = document.createElement('p');
    const ingredientText = recipe.ingredient_list;
    ingredients.textContent =
      ingredientText.length > 140 ? ingredientText.slice(0, 140) + '…' : ingredientText;

    const details = document.createElement('span');
    details.className = 'recipe-time';

    const parts = [];

    if (recipe.servings !== null) {
      parts.push(`${recipe.servings} servings`);
    }

    if (recipe.prep_minutes !== null) {
      parts.push(`Prep: ${recipe.prep_minutes} min`);
    }

    if (recipe.cook_minutes !== null) {
      parts.push(`Cook: ${recipe.cook_minutes} min`);
    }

    details.textContent = parts.join(' · ');

    content.append(title, ingredients, details);
    card.append(image, content);
    recipeGrid.append(card);
  });
}

async function loadRecipes() {
  try {
    const response = await fetch('/api/recipes', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    const body = await response.json();

    if (!response.ok) {
      collectionMessage.textContent = body.error || 'Unable to load your recipes.';
      return;
    }

    if (!Array.isArray(body.recipes)) {
      throw new Error('Invalid recipe collection response');
    }

    allRecipes = body.recipes;
    searchInput.disabled = false;
    filterRecipes();
  } catch (error) {
    collectionMessage.textContent = 'Unable to load your recipes. Please reload the page.';
  }
}

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

// Personaliza a mensagem de boas-vindas
async function loadWelcomeMessage() {
  const heading = document.querySelector('#welcome-heading');
  const description = document.querySelector('#welcome-description');

  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (!response.ok) return;

    const { user } = await response.json();

    if (!user?.display_name || !user?.email) return;

    let isNewAccount = false;

    try {
      isNewAccount = sessionStorage.getItem('cardamom_new_account') === user.email.toLowerCase();

      sessionStorage.removeItem('cardamom_new_account');
    } catch {
      // Use the returning-user greeting if storage is unavailable.
    }

    if (isNewAccount) {
      heading.textContent = 'Welcome to Cardamom!';
      description.textContent = 'Your culinary journey starts here.';
    } else {
      heading.textContent = `Welcome back, ${user.display_name}!`;
      description.textContent = 'What are we cooking today?';
    }
  } catch {
    // Keep the default greeting if the request fails.
  }
}

// Filtra as receitas pelo nome ou ingredientes
function filterRecipes() {
  const search = searchInput.value.trim().toLowerCase();

  const filteredRecipes = allRecipes.filter((recipe) => {
    const name = (recipe.name || '').toLowerCase();
    const ingredients = (recipe.ingredient_list || '').toLowerCase();

    return name.includes(search) || ingredients.includes(search);
  });

  displayRecipes(filteredRecipes);
}

searchInput.addEventListener('input', filterRecipes);

void loadWelcomeMessage();
void loadRecipes();
