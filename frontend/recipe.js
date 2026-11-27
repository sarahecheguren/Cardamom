// University of Florida
// Cardamom Project
// By Gianluca Perrella (frontend contributions)
// CEN3031 Fall, 2026
// Handles recipe details, deletion and favorite interactions.

const publicView = new URLSearchParams(window.location.search).get('source') === 'discover';

if (publicView) {
  const backLink = document.querySelector('.back-link');
  backLink.href = 'discover.html';
  backLink.textContent = '← Back to Discover';
  document.querySelector('.recipe-details-header .eyebrow').textContent = 'The Cardamom community';
}

const recipeName = document.querySelector('#recipe-name');
const recipeMessage = document.querySelector('#recipe-message');
const recipeServings = document.querySelector('#recipe-servings');
const recipePrep = document.querySelector('#recipe-prep');
const recipeCook = document.querySelector('#recipe-cook');
const recipeIngredients = document.querySelector('#recipe-ingredients');
const recipeSteps = document.querySelector('#recipe-steps');
const recipeNotes = document.querySelector('#recipe-notes');
const deleteRecipeButton = document.querySelector('#delete-recipe-button');
const editRecipeLink = document.querySelector('#edit-recipe-link');

// Displays a message if something goes wrong
function showError(message) {
  recipeName.textContent = 'Recipe Unavailable';
  recipeMessage.textContent = message;
}

// Displays the recipe information on the page
function displayRecipe(recipe) {
  recipeName.textContent = recipe.name;

  const photo = document.querySelector('#recipe-photo');

  const imageUrl = validateImageUrl(recipe.image_url);

  if (imageUrl) {
    photo.alt = recipe.name;

    photo.addEventListener('load', () => {
      photo.hidden = false;
    });

    photo.addEventListener('error', () => {
      photo.hidden = true;
    });

    photo.referrerPolicy = 'no-referrer';
    photo.src = `https://${imageUrl.host}${imageUrl.pathname}${imageUrl.search}${imageUrl.hash}`;
  }

  recipeServings.textContent =
    recipe.servings == null ? 'Servings: —' : `Servings: ${recipe.servings}`;

  recipePrep.textContent =
    recipe.prep_minutes == null ? 'Prep time: —' : `Prep: ${recipe.prep_minutes} minutes`;

  recipeCook.textContent =
    recipe.cook_minutes == null ? 'Cook time: —' : `Cook: ${recipe.cook_minutes} minutes`;

  recipeIngredients.textContent = recipe.ingredient_list || 'No ingredients provided.';

  recipeSteps.textContent = recipe.steps || 'No cooking instructions added.';

  recipeNotes.textContent = recipe.notes || 'No additional notes.';

  recipeMessage.textContent = publicView ? `Shared by ${recipe.author}` : '';
  document.title = `${recipe.name} | Cardamom`;

  editRecipeLink.href = `add-recipe.html?id=${encodeURIComponent(recipe.id)}`;
  editRecipeLink.hidden = publicView;
  deleteRecipeButton.hidden = publicView;

  if (publicView) {
    void setupFavorite(recipe.id);
  }
}

// Checks login and loads the selected recipe
async function loadRecipe() {
  const parameters = new URLSearchParams(window.location.search);
  const recipeId = parameters.get('id');

  // Each recipe must have a valid ID
  if (!recipeId || !/^[1-9]\d*$/.test(recipeId)) {
    showError('No valid recipe was selected.');
    return;
  }

  try {
    // Check whether the user is logged in
    const sessionResponse = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (sessionResponse.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!sessionResponse.ok) {
      showError('Unable to check your session. Please try again.');
      return;
    }

    // Request the full recipe from the backend
    const endpoint = publicView ? '/api/discover' : '/api/recipes';
    const response = await fetch(`${endpoint}/${recipeId}`, {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (response.status === 404) {
      showError('Recipe not found, or the recipe details API is not available yet.');
      return;
    }

    if (!response.ok) {
      showError('Unable to load this recipe. Please try again.');
      return;
    }

    const data = await response.json();
    const recipe = data.recipe || data;

    if (!recipe || typeof recipe.name !== 'string') {
      showError('The server returned invalid recipe information.');
      return;
    }

    displayRecipe(recipe);
  } catch (error) {
    showError('Could not connect to the server. Please try again.');
  }
}

// Deletes the selected recipe after user confirmation
async function deleteRecipe() {
  if (publicView) return;
  const parameters = new URLSearchParams(window.location.search);
  const recipeId = parameters.get('id');

  if (!recipeId || !/^[1-9]\d*$/.test(recipeId)) {
    recipeMessage.textContent = 'Invalid recipe ID.';
    return;
  }

  const confirmed = window.confirm(
    `Are you sure you want to delete "${recipeName.textContent}"? This action cannot be undone.`
  );

  if (!confirmed) {
    return;
  }

  deleteRecipeButton.disabled = true;
  recipeMessage.textContent = 'Deleting recipe...';

  try {
    const response = await fetch(`/api/recipes/${recipeId}`, {
      method: 'DELETE',
      credentials: 'same-origin',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (response.ok) {
      window.location.replace('dashboard.html');
      return;
    }

    if (response.status === 404 || response.status === 405) {
      recipeMessage.textContent =
        'Recipe deletion is not available yet, or the recipe was not found.';
    } else {
      recipeMessage.textContent = 'Unable to delete this recipe. Please try again.';
    }
  } catch (error) {
    recipeMessage.textContent = 'Could not connect to the server. Please try again.';
  } finally {
    deleteRecipeButton.disabled = false;
  }
}

// Connect the delete button to the deletion function
deleteRecipeButton.addEventListener('click', deleteRecipe);

// Load the recipe when the page opens
void loadRecipe();

async function setupFavorite(recipeId) {
  const controls = document.querySelector('#favorite-controls');
  const button = document.querySelector('#favorite-button');
  const message = document.querySelector('#favorite-message');

  let saved = false;

  function updateButton() {
    button.textContent = saved ? '♥ Remove from favorites' : '♡ Save to favorites';

    button.classList.toggle('is-saved', saved);
  }

  controls.hidden = false;
  message.textContent = 'Checking favorites...';

  try {
    const options = {
      credentials: 'same-origin',
      cache: 'no-store',
    };

    const [personalResponse, favoritesResponse] = await Promise.all([
      fetch('/api/recipes', options),
      fetch('/api/favorites', options),
    ]);

    if (personalResponse.status === 401 || favoritesResponse.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!personalResponse.ok || !favoritesResponse.ok) {
      throw new Error('Unable to load favorites');
    }

    const personal = await personalResponse.json();
    const favorites = await favoritesResponse.json();

    if (!Array.isArray(personal.recipes) || !Array.isArray(favorites.recipes)) {
      throw new Error('Invalid recipe response');
    }

    // Impede que o usuario salve sua propria receita
    const isOwned = personal.recipes.some((recipe) => String(recipe.id) === String(recipeId));

    if (isOwned) {
      controls.hidden = true;
      return;
    }

    saved = favorites.recipes.some((recipe) => String(recipe.id) === String(recipeId));

    updateButton();
    button.hidden = false;
    message.textContent = '';
  } catch {
    message.textContent = 'Unable to check your favorites.';
    return;
  }

  // Adiciona ou remove a receita dos favoritos
  button.addEventListener('click', async () => {
    button.disabled = true;
    message.textContent = 'Updating favorites...';

    try {
      const response = await fetch(`/api/favorites/${encodeURIComponent(recipeId)}`, {
        method: saved ? 'DELETE' : 'POST',
        credentials: 'same-origin',
      });

      if (response.status === 401) {
        window.location.replace('index.html');
        return;
      }

      const body = await response.json();

      if (!response.ok) {
        message.textContent = body.error || 'Unable to update favorites.';
        return;
      }

      saved = body.favorited === true;

      updateButton();

      message.textContent = saved
        ? 'Recipe saved to your favorites.'
        : 'Recipe removed from your favorites.';
    } catch {
      message.textContent = 'Could not connect to the server. Please try again.';
    } finally {
      button.disabled = false;
    }
  });
}
