// University of Florida
// Cardamom Project
// By Gianluca Perrella (frontend contributions)
// CEN3031 Fall, 2026
// Loads and filters publicly shared community recipes.

const logoutButton = document.querySelector('#logout-button');
const recipeGrid = document.querySelector('#recipe-grid');
const recipeCount = document.querySelector('#recipe-count');
const collectionMessage = document.querySelector('#collection-message');
const searchInput = document.querySelector('#discover-search');

let allRecipes = [];

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
    recipeGrid.append(createSharedRecipeCard(recipe, { community: true, showDetails: true }));
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
