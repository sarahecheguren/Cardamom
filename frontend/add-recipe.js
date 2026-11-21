const recipeForm = document.querySelector('#recipe-form');
const saveButton = recipeForm.querySelector('[type="submit"]');
const message = recipeForm.querySelector('.form-note');
const cancelLink = recipeForm.querySelector('.cancel-link');

// With ?id= in the address, this page edits that recipe instead of adding a new one.
const editId = new URLSearchParams(window.location.search).get('id');
const editing = editId !== null;
const recipePage = editing ? `recipe.html?id=${encodeURIComponent(editId)}` : '';
const saveLabel = editing ? 'Save Changes' : 'Save Recipe';

let sessionReady = false;
let saving = false;

message.setAttribute('role', 'status');
message.setAttribute('aria-live', 'polite');
message.textContent = 'Checking your session...';

function showMessage(text) {
  message.textContent = text;
}

if (editing) {
  document.title = 'Edit Recipe | Cardamom';
  document.querySelector('#page-title').textContent = 'Edit Recipe';
  document.querySelector('#page-intro-text').textContent =
    'Update this recipe with what you learned the last time you made it.';
  saveButton.textContent = saveLabel;
  // Cancelling goes back to the recipe, which is left as it was.
  cancelLink.href = recipePage;
}

// Fills the form with the stored recipe. Returns false if it could not be loaded.
async function loadRecipeForEdit() {
  if (!/^[1-9]\d*$/.test(editId)) {
    cancelLink.href = 'dashboard.html';
    showMessage('No valid recipe was selected.');
    return false;
  }

  const response = await fetch(`/api/recipes/${editId}`, {
    credentials: 'same-origin',
    cache: 'no-store',
  });

  if (response.status === 401) {
    window.location.replace('index.html');
    return false;
  }

  if (response.status === 404) {
    cancelLink.href = 'dashboard.html';
    showMessage('Recipe not found.');
    return false;
  }

  const body = await response.json().catch(() => ({}));
  const recipe = body.recipe;

  if (!response.ok || !recipe || typeof recipe.name !== 'string') {
    showMessage('Unable to load this recipe. Please reload the page.');
    return false;
  }

  const fields = recipeForm.elements;
  fields.name.value = recipe.name;
  fields.ingredient_list.value = recipe.ingredient_list ?? '';
  fields.steps.value = recipe.steps ?? '';
  fields.notes.value = recipe.notes ?? '';
  fields.servings.value = recipe.servings ?? '';
  fields.prep_minutes.value = recipe.prep_minutes ?? '';
  fields.cook_minutes.value = recipe.cook_minutes ?? '';
  return true;
}

async function checkSession() {
  try {
    const response = await fetch('/api/auth/me', {
      credentials: 'same-origin',
      cache: 'no-store',
    });

    if (response.status === 401) {
      window.location.replace('index.html');
      return;
    }

    if (!response.ok) {
      showMessage('Unable to check your session. Please reload the page.');
      return;
    }

    if (editing) {
      showMessage('Loading recipe...');

      if (!(await loadRecipeForEdit())) {
        return;
      }
    }

    sessionReady = true;
    saveButton.disabled = false;
    showMessage('');
  } catch (error) {
    showMessage('Cannot reach the server. Please reload the page.');
  }
}

recipeForm.addEventListener('submit', async (event) => {
  event.preventDefault();

  if (!sessionReady || saving || !recipeForm.reportValidity()) {
    return;
  }

  const data = new FormData(recipeForm);

  const optionalNumber = (field) => {
    const value = data.get(field).trim();
    return value === '' ? null : Number(value);
  };

  const recipe = {
    name: data.get('name').trim(),
    ingredient_list: data.get('ingredient_list').trim(),
    steps: data.get('steps').trim(),
    notes: data.get('notes').trim(),
    servings: optionalNumber('servings'),
    prep_minutes: optionalNumber('prep_minutes'),
    cook_minutes: optionalNumber('cook_minutes'),
  };

  if (!recipe.name || !recipe.ingredient_list) {
    showMessage('Recipe name and ingredients are required.');
    return;
  }

  saving = true;
  saveButton.disabled = true;
  saveButton.textContent = 'Saving...';
  showMessage('');

  try {
    const response = await fetch(editing ? `/api/recipes/${editId}` : '/api/recipes', {
      method: editing ? 'PUT' : 'POST',
      credentials: 'same-origin',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(recipe),
    });

    const body = await response.json().catch(() => ({}));

    if (response.status === 401) {
      sessionReady = false;
      window.location.replace('index.html');
      return;
    }

    if (!response.ok) {
      showMessage(body.error || 'Unable to save the recipe. Please try again.');

      // The server refused the recipe because the plan is full.
      if (body.limit_reached) {
        const plansLink = document.createElement('a');
        plansLink.href = 'plans.html';
        plansLink.textContent = 'Compare plans';
        message.append(' ', plansLink);
      }

      return;
    }

    window.location.replace(editing ? recipePage : 'dashboard.html');
  } catch (error) {
    showMessage('Could not confirm whether the recipe was saved. Check before trying again.');
  } finally {
    saving = false;
    saveButton.disabled = !sessionReady;
    saveButton.textContent = saveLabel;
  }
});

checkSession();
