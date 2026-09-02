const banner = document.getElementById("banner");
const tagListEl = document.getElementById("tag-list");
const categorySelect = document.getElementById("category");
const form = document.getElementById("item-form");
const imageInput = document.getElementById("image-input");
const imagePreview = document.getElementById("image-preview");
const autofillButton = document.getElementById("autofill-button");
const autofillStatus = document.getElementById("autofill-status");

let tags = [];
let selectedImageDataUrl = null;

// Phone camera photos can be several MB / thousands of pixels wide; shrink
// to a reasonable max dimension before base64-encoding so uploads stay fast
// and don't risk hitting a server body-size limit.
function resizeImageToDataUrl(file, maxDimension = 1280, quality = 0.82) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onerror = () => reject(new Error("Could not read the image file."));
    reader.onload = () => {
      const img = new Image();
      img.onerror = () => reject(new Error("Could not decode the image file."));
      img.onload = () => {
        const scale = Math.min(1, maxDimension / Math.max(img.width, img.height));
        const canvas = document.createElement("canvas");
        canvas.width = Math.round(img.width * scale);
        canvas.height = Math.round(img.height * scale);
        const ctx = canvas.getContext("2d");
        ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
        resolve(canvas.toDataURL("image/jpeg", quality));
      };
      img.src = reader.result;
    };
    reader.readAsDataURL(file);
  });
}

imageInput.addEventListener("change", async () => {
  const file = imageInput.files[0];
  if (!file) return;
  try {
    selectedImageDataUrl = await resizeImageToDataUrl(file);
    imagePreview.src = selectedImageDataUrl;
    imagePreview.hidden = false;
    autofillButton.hidden = false;
    // Every photo pick (first one or a replacement) re-runs the AI fill-in,
    // overwriting whatever is currently in Name/Description.
    autofillButton.click();
  } catch (err) {
    showBanner(err.message, "error");
  }
});

autofillButton.addEventListener("click", async () => {
  if (!selectedImageDataUrl) return;
  autofillButton.disabled = true;
  autofillStatus.hidden = false;
  autofillStatus.textContent = "Analyzing photo…";
  try {
    const suggestion = await api("/vision/describe-item", {
      method: "POST",
      body: JSON.stringify({ image_base64: selectedImageDataUrl }),
    });
    if (suggestion.name) document.getElementById("name").value = suggestion.name;
    if (suggestion.description) document.getElementById("description").value = suggestion.description;
    autofillStatus.textContent = "Filled in from photo — review before saving.";
  } catch (err) {
    autofillStatus.textContent = "";
    autofillStatus.hidden = true;
    showBanner(err.message, "error");
  } finally {
    autofillButton.disabled = false;
  }
});

function showBanner(message, kind) {
  banner.textContent = message;
  banner.className = `banner ${kind}`;
  banner.hidden = false;
}

function hideBanner() {
  banner.hidden = true;
}

async function api(path, options) {
  const response = await fetch(`/api${path}`, {
    headers: { "Content-Type": "application/json" },
    ...options,
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(body.error || `request failed (${response.status})`);
  }
  return body;
}

function renderTags(checkedIds = new Set()) {
  tagListEl.innerHTML = "";
  if (tags.length === 0) {
    const li = document.createElement("li");
    li.className = "empty";
    li.textContent = "No tags yet — add one below.";
    tagListEl.appendChild(li);
    return;
  }
  for (const tag of tags) {
    const li = document.createElement("li");
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.value = tag.id;
    checkbox.id = `tag-${tag.id}`;
    checkbox.checked = checkedIds.has(tag.id);
    const swatch = document.createElement("span");
    swatch.className = "tag-swatch";
    swatch.style.background = tag.color;
    const label = document.createElement("label");
    label.htmlFor = checkbox.id;
    label.textContent = tag.name;
    li.appendChild(checkbox);
    li.appendChild(swatch);
    li.appendChild(label);
    tagListEl.appendChild(li);
  }
}

function checkedTagIds() {
  return Array.from(tagListEl.querySelectorAll("input[type=checkbox]:checked")).map((el) => Number(el.value));
}

async function loadCategories() {
  const categories = await api("/categories");
  categorySelect.innerHTML = '<option value="">None</option>';
  for (const category of categories) {
    const option = document.createElement("option");
    option.value = category.id;
    option.textContent = category.name;
    categorySelect.appendChild(option);
  }
}

async function loadTags() {
  const checked = new Set(checkedTagIds());
  tags = await api("/tags");
  renderTags(checked);
}

document.getElementById("add-tag-button").addEventListener("click", async () => {
  const input = document.getElementById("new-tag-name");
  const name = input.value.trim();
  if (!name) return;
  try {
    await api("/tags", { method: "POST", body: JSON.stringify({ name }) });
    input.value = "";
    await loadTags();
  } catch (err) {
    showBanner(err.message, "error");
  }
});

form.addEventListener("submit", async (event) => {
  event.preventDefault();
  hideBanner();

  const name = document.getElementById("name").value.trim();
  if (!name) {
    showBanner("Name is required.", "error");
    return;
  }

  const payload = {
    name,
    description: document.getElementById("description").value.trim(),
    quantity: Number(document.getElementById("quantity").value) || 0,
    location: document.getElementById("location").value.trim(),
    status: document.getElementById("status").value,
    category_id: categorySelect.value ? Number(categorySelect.value) : null,
    tag_ids: checkedTagIds(),
  };

  try {
    const created = await api("/items", { method: "POST", body: JSON.stringify(payload) });
    if (selectedImageDataUrl) {
      await api(`/items/${created.id}/image`, {
        method: "POST",
        body: JSON.stringify({ image_base64: selectedImageDataUrl }),
      });
    }
    showBanner(`"${name}" was added.`, "success");
    form.reset();
    renderTags();
    selectedImageDataUrl = null;
    imagePreview.hidden = true;
    imagePreview.src = "";
    autofillButton.hidden = true;
    autofillStatus.hidden = true;
    autofillStatus.textContent = "";
  } catch (err) {
    showBanner(err.message, "error");
  }
});

Promise.all([loadCategories(), loadTags()]).catch((err) => showBanner(err.message, "error"));
