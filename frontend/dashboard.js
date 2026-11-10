const logoutButton = document.querySelector('#logout-button');
const recipeGrid = document.querySelector('#recipe-grid');
const recipeCount = document.querySelector('#recipe-count');
const collectionMessage = document.querySelector('#collection-message');

function displayRecipes(recipes) {
  recipeGrid.replaceChildren();
  recipeCount.textContent = `${recipes.length} ${recipes.length === 1 ? 'recipe' : 'recipes'}`;

  if (recipes.length === 0) {
    collectionMessage.textContent = 'Your collection is empty. Use + Add Recipe to get started.';
    return;
  }

  collectionMessage.textContent = '';

  recipes.forEach((recipe) => {
    const card = document.createElement('article');
    card.className = 'recipe-card';

    const image = document.createElement('div');
    image.className = 'recipe-image';
    image.textContent = recipe.name;

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

    displayRecipes(body.recipes);
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

void loadRecipes();
