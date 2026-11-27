// University of Florida
// Cardamom Project
// By Gianluca Perrella (frontend contributions)
// CEN3031 Fall, 2026
// Handles the personal dashboard, recipe search and welcome messages.

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
    recipeGrid.append(createSharedRecipeCard(recipe, { showDetails: true }));
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
