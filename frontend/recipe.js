const recipeName = document.querySelector('#recipe-name');
const recipeMessage = document.querySelector('#recipe-message');
const recipeServings = document.querySelector('#recipe-servings');
const recipePrep = document.querySelector('#recipe-prep');
const recipeCook = document.querySelector('#recipe-cook');
const recipeIngredients = document.querySelector('#recipe-ingredients');
const recipeSteps = document.querySelector('#recipe-steps');
const recipeNotes = document.querySelector('#recipe-notes');

// Displays a message if something goes wrong
function showError(message) {
  recipeName.textContent = 'Recipe Unavailable';
  recipeMessage.textContent = message;
}

// Displays the recipe information on the page
function displayRecipe(recipe) {
  recipeName.textContent = recipe.name;

  recipeServings.textContent =
    recipe.servings == null ? 'Servings: —' : `Servings: ${recipe.servings}`;

  recipePrep.textContent =
    recipe.prep_minutes == null ? 'Prep time: —' : `Prep: ${recipe.prep_minutes} minutes`;

  recipeCook.textContent =
    recipe.cook_minutes == null ? 'Cook time: —' : `Cook: ${recipe.cook_minutes} minutes`;

  recipeIngredients.textContent = recipe.ingredient_list || 'No ingredients provided.';

  recipeSteps.textContent = recipe.steps || 'No cooking instructions added.';

  recipeNotes.textContent = recipe.notes || 'No additional notes.';

  recipeMessage.textContent = '';
  document.title = `${recipe.name} | Cardamom`;
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
    const response = await fetch(`/api/recipes/${recipeId}`, {
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

// Load the recipe when the page opens
void loadRecipe();
