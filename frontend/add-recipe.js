const recipeForm = document.querySelector('#recipe-form');
const saveButton = recipeForm.querySelector('[type="submit"]');
const message = recipeForm.querySelector('.form-note');

let sessionReady = false;
let saving = false;

message.setAttribute('role', 'status');
message.setAttribute('aria-live', 'polite');
message.textContent = 'Checking your session...';

function showMessage(text) {
  message.textContent = text;
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
    const response = await fetch('/api/recipes', {
      method: 'POST',
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
      return;
    }

    window.location.replace('dashboard.html');
  } catch (error) {
    showMessage('Could not confirm whether the recipe was saved. Check before trying again.');
  } finally {
    saving = false;
    saveButton.disabled = !sessionReady;
    saveButton.textContent = 'Save Recipe';
  }
});

checkSession();
