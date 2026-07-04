let candidates = [];
let latestRanking = [];

const pages = document.querySelectorAll('.page');
const navLinks = document.querySelectorAll('.nav-link');
const modal = document.querySelector('#candidate-modal');

function showPage(name) {
  pages.forEach(page => page.classList.toggle('active', page.id === name));
  navLinks.forEach(link => link.classList.toggle('active', link.dataset.page === name));
  const titles = {dashboard:'Good candidates, found faster.',resumes:'Candidate workspace',matching:'Match skills to opportunity.'};
  document.querySelector('#page-title').textContent = titles[name];
}

navLinks.forEach(link => link.addEventListener('click', () => showPage(link.dataset.page)));
document.querySelectorAll('[data-go]').forEach(button => button.addEventListener('click', () => showPage(button.dataset.go)));
document.querySelector('#add-top').addEventListener('click', () => modal.showModal());
document.querySelector('#close-modal').addEventListener('click', () => modal.close());

function toast(message) {
  const node = document.querySelector('#toast'); node.textContent = message; node.classList.add('show');
  setTimeout(() => node.classList.remove('show'), 2300);
}

function initials(name) { return name.split(' ').map(v => v[0]).join('').slice(0,2).toUpperCase(); }
function escapeHtml(value) { const node=document.createElement('div'); node.textContent=value; return node.innerHTML; }

function candidateCard(candidate) {
  const skills = candidate.skills.slice(0,5).map(s => `<span class="skill">${escapeHtml(s)}</span>`).join('');
  return `<article class="candidate-card" data-search="${escapeHtml((candidate.name+' '+candidate.skills.join(' ')).toLowerCase())}"><button class="delete" data-delete="${candidate.id}" title="Remove candidate">×</button><div class="candidate-top"><span class="avatar">${initials(candidate.name)}</span><div><h4>${escapeHtml(candidate.name)}</h4><p>${escapeHtml(candidate.email)}</p></div></div><div class="candidate-meta"><span>◷ ${candidate.experience} years</span><span>⌑ ${escapeHtml(candidate.education)}</span></div><div class="skill-list">${skills || '<span class="skill">No skills detected</span>'}</div></article>`;
}

async function loadCandidates() {
  const response = await fetch('/api/resumes');
  const data = await response.json(); candidates = data.resumes;
  document.querySelector('#total-count').textContent = data.count;
  document.querySelector('#skill-count').textContent = new Set(candidates.flatMap(c => c.skills)).size;
  document.querySelector('#recent-list').innerHTML = candidates.slice(-3).reverse().map(candidateCard).join('');
  document.querySelector('#candidate-list').innerHTML = candidates.map(candidateCard).join('') || '<div class="empty-state"><h3>No candidates yet</h3></div>';
}

document.addEventListener('click', async event => {
  const id = event.target.dataset.delete;
  if (!id || !confirm('Remove this candidate?')) return;
  await fetch(`/api/resumes?id=${id}`, {method:'DELETE'}); await loadCandidates(); toast('Candidate removed');
});

document.querySelector('#candidate-search').addEventListener('input', event => {
  const term = event.target.value.toLowerCase();
  document.querySelectorAll('#candidate-list .candidate-card').forEach(card => card.hidden = !card.dataset.search.includes(term));
});

document.querySelector('#candidate-form').addEventListener('submit', async event => {
  event.preventDefault(); const body = new URLSearchParams(new FormData(event.target));
  const response = await fetch('/api/resumes',{method:'POST',body});
  if (response.ok) { event.target.reset(); modal.close(); await loadCandidates(); toast('Candidate added and skills indexed'); }
  else toast('Please check the candidate details');
});

document.querySelector('#job-form').addEventListener('submit', async event => {
  event.preventDefault();
  const response = await fetch('/api/rank',{method:'POST',body:new URLSearchParams(new FormData(event.target))});
  const data = await response.json(); latestRanking = data.results || [];
  const root = document.querySelector('#ranking-results'); root.className='ranking-list';
  root.innerHTML = latestRanking.map(r => {
    const matched = r.matched.map(skill => `<span class="skill-good">${escapeHtml(skill)}</span>`).join('');
    const missing = r.missing.map(skill => `<span class="skill-gap">${escapeHtml(skill)}</span>`).join('');
    const guidance = r.missing.length
      ? `To become a stronger fit for this role, focus on: ${escapeHtml(r.missing.join(', '))}.`
      : 'This candidate meets every listed skill requirement.';
    return `<article class="rank-card"><div class="rank-main"><span class="rank-number">#${r.rank}</span><div class="rank-person"><h4>${escapeHtml(r.name)}</h4><p>${r.experience} years experience · ${escapeHtml(r.email)}</p></div><div class="score">${r.score.toFixed(0)}<small>match score</small></div></div><div class="skill-analysis"><div><b>Matched skills</b><div class="analysis-tags">${matched || '<span class="neutral-tag">None detected</span>'}</div></div><div><b>Skills to improve</b><div class="analysis-tags">${missing || '<span class="complete-tag">No skill gaps</span>'}</div></div></div><p class="guidance"><strong>Recommendation:</strong> ${guidance}</p></article>`;
  }).join('') || '<div class="empty-state"><h3>No candidates found</h3></div>';
  document.querySelector('#export-btn').disabled = !latestRanking.length; toast('Ranking complete');
});

document.querySelector('#export-btn').addEventListener('click', () => {
  const text = ['TALENTLENS SHORTLIST','',...latestRanking.map(r=>`#${r.rank} ${r.name} — ${r.score}% match\nExperience: ${r.experience} years\nMatched: ${r.matched.join(', ') || 'None'}\nMissing: ${r.missing.join(', ') || 'None'}\n`)].join('\n');
  const link=document.createElement('a'); link.href=URL.createObjectURL(new Blob([text],{type:'text/plain'})); link.download='shortlist.txt'; link.click(); URL.revokeObjectURL(link.href);
});

loadCandidates().catch(() => toast('Could not connect to the C++ server'));
