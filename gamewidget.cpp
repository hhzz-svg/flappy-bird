#include "gamewidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QRandomGenerator>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSettings>
#include <QStringList>
#include <QtMath>

// ==========================================================================
//  small helpers
// ==========================================================================
static inline qreal frand(qreal lo, qreal hi)
{
    return lo + (hi - lo) * QRandomGenerator::global()->generateDouble();
}
static inline QColor shift(const QColor &c, int d)
{
    return QColor(qBound(0, c.red()   + d, 255),
                  qBound(0, c.green() + d, 255),
                  qBound(0, c.blue()  + d, 255));
}
static int getBest(const QString &id)
{
    QSettings s(QStringLiteral("FlappyQt"), QStringLiteral("FlappyBird"));
    return s.value(QStringLiteral("best/") + id, 0).toInt();
}
static void setBest(const QString &id, int v)
{
    QSettings s(QStringLiteral("FlappyQt"), QStringLiteral("FlappyBird"));
    s.setValue(QStringLiteral("best/") + id, v);
}

// ==========================================================================
//  static data tables
// ==========================================================================
const QVector<GameMode> &GameWidget::modes()
{
    static const QVector<GameMode> m = {
        //           id          name    icon   desc                                   accent               grav  jump  gap  spd  spawn mul  ramp  move  fog   storm laser rate
        { QStringLiteral("classic"), QStringLiteral("经典"),  QStringLiteral("🐤"),  QStringLiteral("原汁原味，稳定难度"),      QColor(255,217, 61), 0.42,-7.4,168,2.60,1600,1.0,0.6, false,false,false,false,0.50 },
        { QStringLiteral("zen"),     QStringLiteral("禅意"),  QStringLiteral("🍃"),  QStringLiteral("宽间距 · 慢节奏 · 放松练习"), QColor(126,232,176),0.34,-6.7,214,2.05,2100,1.0,0.0, false,false,false,false,0.65 },
        { QStringLiteral("turbo"),   QStringLiteral("极速"),  QStringLiteral("🔥"),  QStringLiteral("越来越快 · 得分 ×2"),        QColor(255,107, 61), 0.50,-8.1,182,3.50,1350,2.0,1.4, false,false,false,false,0.50 },
        { QStringLiteral("night"),   QStringLiteral("夜航"),  QStringLiteral("🌙"),  QStringLiteral("视野受限 · 只有身边看得见"),   QColor(127,211,255),0.42,-7.4,182,2.55,1600,1.5,0.5, false,true, false,false,0.55 },
        { QStringLiteral("storm"),   QStringLiteral("风暴"),  QStringLiteral("⛈"), QStringLiteral("管道晃动 · 阵风来袭 · 闪电"), QColor(197,139,255),0.44,-7.6,190,2.75,1650,1.8,0.7, true, false,true, false,0.50 },
        { QStringLiteral("laser"),   QStringLiteral("激光"),  QStringLiteral("⚡"),  QStringLiteral("移动激光门 · 抓准间隙"),      QColor(255, 80,120), 0.40,-7.3,176,2.50,1750,2.0,0.5, false,false,false,true, 0.55 },
    };
    return m;
}

const QVector<Skin> &GameWidget::skins()
{
    static const QVector<Skin> s = {
        { QStringLiteral("default"), QStringLiteral("经典黄"),   0, QColor(255,217, 61), false, false },
        { QStringLiteral("red"),     QStringLiteral("赤焰"),    20, QColor(255, 90, 60), false, false },
        { QStringLiteral("blue"),    QStringLiteral("蔚蓝"),    20, QColor( 90,160,255), false, false },
        { QStringLiteral("green"),   QStringLiteral("青柠"),    30, QColor( 99,214,160), false, false },
        { QStringLiteral("purple"),  QStringLiteral("魅紫"),    40, QColor(197,139,255), false, false },
        { QStringLiteral("neon"),    QStringLiteral("霓虹"),    60, QColor(140,224,255), false, true  },
        { QStringLiteral("gold"),    QStringLiteral("黄金"),   100, QColor(255,200, 60), false, false },
        { QStringLiteral("rainbow"), QStringLiteral("彩虹"),   200, QColor(255,255,255), true,  true  },
    };
    return s;
}

const Skin &GameWidget::skin() const
{
    const auto &all = skins();
    for (const auto &s : all) if (s.id == m_skinId) return s;
    return all[0];
}

QColor GameWidget::birdColor() const
{
    const Skin &s = skin();
    if (s.rainbow)
        return QColor::fromHsvF(std::fmod(m_tGlobal * 0.15, 1.0), 0.68, 1.0);
    return s.color;
}

// ==========================================================================
//  construction
// ==========================================================================
GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
    , m_state(Menu)
    , m_modeIndex(0)
    , m_menuIndex(0)
    , m_shopIndex(0)
    , m_birdY(LH / 2.0)
    , m_birdV(0)
    , m_birdRot(0)
    , m_wingPhase(0)
    , m_groundOffset(0)
    , m_bgOffset(0)
    , m_tGlobal(0)
    , m_score(0)
    , m_runCoins(0)
    , m_combo(0)
    , m_bestCombo(0)
    , m_elapsedSec(0)
    , m_screenShake(0)
    , m_scorePop(0)
    , m_flash(0)
    , m_msSincePipe(0)
    , m_newRecord(false)
    , m_gustActive(false)
    , m_gustTimer(2.5)
    , m_gustAccel(0)
    , m_gustWarn(0)
    , m_gustDir(0)
    , m_coinsBalance(0)
    , m_skinId(QStringLiteral("default"))
    , m_shopMsgLife(0)
    , m_timer(new QTimer(this))
{
    setMinimumSize(360, 540);
    resize(LW, LH);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    for (int i = 0; i < 90; ++i)
        m_stars.append({ frand(0, LW), frand(0, (LH - GROUND_H) * 0.8),
                         frand(0.4, 2.0), frand(0, M_PI * 2), frand(0.02, 0.07) });
    for (int i = 0; i < 6; ++i)
        m_clouds.append({ frand(0, LW), frand(40, LH * 0.42),
                          frand(0.55, 1.45), frand(0.12, 0.47) });
    for (int i = 0; i < 5; ++i)
        m_hills.append({ i * 130.0, frand(150, 240), frand(70, 140) });

    loadProfile();
    resetRun();

    connect(m_timer, &QTimer::timeout, this, &GameWidget::tick);
    m_timer->start(16);
    m_elapsed.start();
}

// ==========================================================================
//  persistence
// ==========================================================================
void GameWidget::loadProfile()
{
    QSettings s(QStringLiteral("FlappyQt"), QStringLiteral("FlappyBird"));
    m_coinsBalance = s.value(QStringLiteral("coins"), 0).toInt();
    m_skinId = s.value(QStringLiteral("skin"), QStringLiteral("default")).toString();
    const QStringList owned = s.value(QStringLiteral("owned"),
                                      QStringList{ QStringLiteral("default") }).toStringList();
    m_owned = QSet<QString>(owned.begin(), owned.end());
    m_owned.insert(QStringLiteral("default"));
    if (!m_owned.contains(m_skinId)) m_skinId = QStringLiteral("default");
}
void GameWidget::saveCoins()
{
    QSettings s(QStringLiteral("FlappyQt"), QStringLiteral("FlappyBird"));
    s.setValue(QStringLiteral("coins"), m_coinsBalance);
}
void GameWidget::saveSkins()
{
    QSettings s(QStringLiteral("FlappyQt"), QStringLiteral("FlappyBird"));
    s.setValue(QStringLiteral("owned"), QStringList(m_owned.begin(), m_owned.end()));
    s.setValue(QStringLiteral("skin"), m_skinId);
}
void GameWidget::saveBest()
{
    if (m_score > getBest(mode().id)) { setBest(mode().id, m_score); m_newRecord = true; }
}

// ==========================================================================
//  lifecycle
// ==========================================================================
void GameWidget::resetRun()
{
    m_birdY = LH / 2.0 - 30;
    m_birdV = 0;
    m_birdRot = 0;
    m_pipes.clear();
    m_lasers.clear();
    m_coins.clear();
    m_particles.clear();
    m_floaters.clear();
    m_msSincePipe = 0;
    m_groundOffset = 0;
    m_score = 0;
    m_runCoins = 0;
    m_combo = 0;
    m_bestCombo = 0;
    m_elapsedSec = 0;
    m_screenShake = 0;
    m_scorePop = 0;
    m_flash = 0;
    m_newRecord = false;
    m_gustActive = false;
    m_gustTimer = 2.5;
    m_gustWarn = 0;
    m_gustDir = 0;
}

void GameWidget::startMode(int index)
{
    m_modeIndex = index;
    resetRun();
    m_state = Ready;
}

void GameWidget::flap()
{
    m_birdV = mode().jump;
    spawnParticles(BIRD_X - 6, m_birdY + 6, 3, { QColor(255,255,255), QColor(223,233,255) }, 2);
}

void GameWidget::die()
{
    if (m_state != Playing) return;
    m_state = GameOver;
    m_screenShake = 16;
    spawnParticles(BIRD_X, m_birdY, 42,
                   { birdColor(), QColor(255,217,61), QColor(255,140,0), QColor(255,85,85) }, 6);
    saveBest();
    if (m_runCoins > 0) { m_coinsBalance += m_runCoins; saveCoins(); }
}

// ==========================================================================
//  difficulty
// ==========================================================================
qreal GameWidget::curSpeed() const
{
    return mode().speed + qMin(3.2, (m_score / 6.0) * 0.35 * mode().ramp);
}
qreal GameWidget::curGap() const
{
    const qreal shrink = mode().ramp > 0 ? qMin(46.0, std::floor(m_score / 5.0) * 4.0) : 0.0;
    return qMax(mode().gap - shrink, 130.0);
}
int GameWidget::curSpawnMs() const
{
    const int t = mode().spawnMs - int(qMin(360.0, std::floor(m_score / 8.0) * 50.0 * mode().ramp));
    return qMax(1050, t);
}

// ==========================================================================
//  spawning
// ==========================================================================
void GameWidget::spawnPipe()
{
    const qreal gap = curGap();
    const qreal minTop = 64;
    const qreal maxTop = LH - GROUND_H - gap - 64;
    const qreal baseTop = minTop + frand(0, qMax(1.0, maxTop - minTop));
    Pipe p;
    p.x = LW + PIPE_W;
    p.baseTop = baseTop;
    p.gap = gap;
    p.scored = false;
    p.amp = mode().movingPipes ? (24 + frand(0, 30) + qMin<qreal>(30, m_score)) : 0;
    p.phase = frand(0, M_PI * 2);
    p.freq = frand(0.9, 1.6);
    p.hue = int(frand(96, 142));
    m_pipes.append(p);

    if (frand(0, 1) < mode().coinRate)
        m_coins.append({ p.x + PIPE_W / 2 + 40, baseTop + gap / 2 + frand(-gap / 2 + 24, gap / 2 - 24), false, 0 });
}

void GameWidget::spawnLaser()
{
    const qreal gap = curGap();
    const qreal amp = 40 + frand(0, 40) + qMin<qreal>(40, m_score);
    const qreal minC = 90 + amp;
    const qreal maxC = LH - GROUND_H - 90 - amp;
    Laser l;
    l.x = LW + LASER_W;
    l.baseCenter = frand(minC, qMax(minC + 1, maxC));
    l.gap = gap;
    l.scored = false;
    l.amp = amp;
    l.phase = frand(0, M_PI * 2);
    l.freq = frand(0.7, 1.2);
    l.blink = frand(0, 2.5);
    m_lasers.append(l);

    if (frand(0, 1) < mode().coinRate)
        m_coins.append({ l.x + 40, l.baseCenter, false, 0 });
}

void GameWidget::spawnParticles(qreal x, qreal y, int count, const QVector<QColor> &cols, qreal spread)
{
    for (int i = 0; i < count; ++i) {
        const qreal a = frand(0, M_PI * 2);
        const qreal sp = 1 + frand(0, spread);
        m_particles.append({ x, y, qCos(a) * sp, qSin(a) * sp,
                             1.0, frand(0.012, 0.032), frand(2, 6),
                             cols[QRandomGenerator::global()->bounded(cols.size())] });
    }
}
void GameWidget::addFloater(qreal x, qreal y, const QString &t, QColor c)
{
    m_floaters.append({ x, y, 1.0, t, c });
}

// ==========================================================================
//  obstacle geometry / collision
// ==========================================================================
qreal GameWidget::pipeTopOf(const Pipe &p) const
{
    return p.baseTop + (p.amp != 0 ? qSin(m_tGlobal * p.freq + p.phase) * p.amp : 0.0);
}
qreal GameWidget::laserCenterOf(const Laser &l) const
{
    return l.baseCenter + qSin(m_tGlobal * l.freq + l.phase) * l.amp;
}
bool GameWidget::laserOn(const Laser &l) const
{
    return std::fmod(l.blink, 2.5) < 2.0;   // ~2.0s on, ~0.5s off
}

static bool circleRectHit(qreal cx, qreal cy, qreal r, qreal rx, qreal ry, qreal rw, qreal rh)
{
    const qreal nx = qBound(rx, cx, rx + rw);
    const qreal ny = qBound(ry, cy, ry + rh);
    const qreal dx = cx - nx, dy = cy - ny;
    return dx * dx + dy * dy < r * r;
}

bool GameWidget::checkCollision()
{
    const qreal r = BIRD_R - 3;
    if (m_birdY + r >= LH - GROUND_H) { m_birdY = LH - GROUND_H - r; return true; }
    if (m_birdY - r <= 0) return true;

    for (const auto &p : m_pipes) {
        if (p.x + PIPE_W < BIRD_X - r || p.x > BIRD_X + r) continue;
        const qreal top = pipeTopOf(p);
        if (circleRectHit(BIRD_X, m_birdY, r, p.x, -50, PIPE_W, top + 50)) return true;
        const qreal by = top + p.gap;
        if (circleRectHit(BIRD_X, m_birdY, r, p.x, by, PIPE_W, LH)) return true;
    }
    for (const auto &l : m_lasers) {
        if (!laserOn(l)) continue;
        if (l.x + LASER_W < BIRD_X - r || l.x > BIRD_X + r) continue;
        const qreal c = laserCenterOf(l);
        const qreal top = c - l.gap / 2, bot = c + l.gap / 2;
        if (circleRectHit(BIRD_X, m_birdY, r, l.x, -50, LASER_W, top + 50)) return true;
        if (circleRectHit(BIRD_X, m_birdY, r, l.x, bot, LASER_W, LH)) return true;
    }
    return false;
}

// ==========================================================================
//  main update
// ==========================================================================
void GameWidget::tick()
{
    m_tGlobal += 0.0166;
    m_wingPhase += 0.25;

    for (auto &c : m_clouds) {
        c.x -= c.speed;
        if (c.x < -70 * c.scale) { c.x = LW + 40; c.y = frand(40, LH * 0.42); }
    }
    for (auto &s : m_stars) s.twinkle += s.speed;

    for (int i = m_particles.size() - 1; i >= 0; --i) {
        Particle &p = m_particles[i];
        p.x += p.vx; p.y += p.vy; p.vy += 0.18;
        p.life -= p.decay;
        if (p.life <= 0) m_particles.removeAt(i);
    }
    for (int i = m_floaters.size() - 1; i >= 0; --i) {
        Floater &f = m_floaters[i];
        f.y -= 0.7; f.life -= 0.018;
        if (f.life <= 0) m_floaters.removeAt(i);
    }
    if (m_scorePop > 0)  m_scorePop  = qMax(0.0, m_scorePop - 0.06);
    if (m_screenShake > 0) { m_screenShake *= 0.9; if (m_screenShake < 0.4) m_screenShake = 0; }
    if (m_flash > 0)     m_flash     = qMax(0.0, m_flash - 0.04);
    if (m_shopMsgLife > 0) m_shopMsgLife = qMax(0.0, m_shopMsgLife - 0.0166);

    if (m_state == Menu) {
        m_birdY = LH / 2.0 + qSin(m_tGlobal * 2.2) * 14;
        m_bgOffset += 0.3;
        update();
        return;
    }
    if (m_state == Ready) {
        m_birdY = LH / 2.0 - 30 + qSin(m_tGlobal * 4) * 8;
        update();
        return;
    }
    if (m_state == Shop || m_state == Paused || m_state == GameOver) { update(); return; }

    // ---- Playing ----
    m_elapsedSec += 0.0166;

    if (mode().storm) {
        m_gustTimer -= 0.0166;
        if (m_gustWarn > 0) m_gustWarn -= 0.0166;
        if (!m_gustActive && m_gustTimer <= 0.6 && m_gustWarn <= 0) m_gustWarn = 0.6;
        if (!m_gustActive && m_gustTimer <= 0) {
            m_gustActive = true;
            m_gustDir = frand(0, 1) < 0.55 ? 1 : -1;
            m_gustAccel = (0.12 + frand(0, 0.1)) * m_gustDir;
            m_gustTimer = 0.9 + frand(0, 0.6);
        } else if (m_gustActive && m_gustTimer <= 0) {
            m_gustActive = false;
            m_gustTimer = 2.6 + frand(0, 2.4);
        }
        if (frand(0, 1) < 0.004) m_flash = 0.85;
    }

    m_birdV += mode().gravity;
    if (m_gustActive) m_birdV += m_gustAccel;
    if (m_birdV > MAX_V) m_birdV = MAX_V;
    m_birdY += m_birdV;
    const qreal targetRot = qBound(-0.5, m_birdV * 0.09, 1.0);
    m_birdRot += (targetRot - m_birdRot) * 0.15;

    m_msSincePipe += 16;
    if (m_msSincePipe >= curSpawnMs()) {
        m_msSincePipe = 0;
        if (mode().lasers) spawnLaser(); else spawnPipe();
    }

    const qreal sp = curSpeed();

    auto onScore = [&]() {
        m_score += qRound(mode().scoreMul);
        m_scorePop = 1;
        if (m_score > 0 && m_score % 10 == 0) {
            spawnParticles(LW / 2, LH * 0.4, 26,
                           { mode().accent, QColor(255,217,61), QColor(255,255,255) }, 6);
            m_screenShake = qMax(m_screenShake, 6.0);
        }
    };

    for (int i = m_pipes.size() - 1; i >= 0; --i) {
        Pipe &p = m_pipes[i];
        p.x -= sp;
        if (!p.scored && p.x + PIPE_W < BIRD_X) { p.scored = true; onScore(); }
        if (p.x + PIPE_W < -20) m_pipes.removeAt(i);
    }
    for (int i = m_lasers.size() - 1; i >= 0; --i) {
        Laser &l = m_lasers[i];
        l.x -= sp;
        l.blink += 0.0166;
        if (!l.scored && l.x + LASER_W < BIRD_X) { l.scored = true; onScore(); }
        if (l.x + LASER_W < -20) m_lasers.removeAt(i);
    }

    for (int i = m_coins.size() - 1; i >= 0; --i) {
        Coin &c = m_coins[i];
        c.x -= sp;
        if (c.x < -40) { m_coins.removeAt(i); continue; }
        if (c.collected) continue;
        c.spin += 0.2;
        const qreal dx = c.x - BIRD_X, dy = c.y - m_birdY;
        if (dx * dx + dy * dy < (BIRD_R + 11) * (BIRD_R + 11)) {
            c.collected = true;
            m_runCoins++; m_combo++;
            m_bestCombo = qMax(m_bestCombo, m_combo);
            spawnParticles(c.x, c.y, 8, { QColor(255,226,122), QColor(255,244,194), QColor(255,217,61) }, 3);
            if (m_combo >= 3) { addFloater(c.x, c.y, QStringLiteral("+%1 ×%2").arg(m_combo).arg(m_combo), QColor(255,226,122)); m_score += 1; }
            else addFloater(c.x, c.y, QStringLiteral("+1"), QColor(255,226,122));
        }
    }

    m_groundOffset += sp;
    m_bgOffset += sp * 0.25;

    if (checkCollision()) die();
    update();
}

// ==========================================================================
//  input
// ==========================================================================
QPointF GameWidget::toLogical(const QPointF &p) const
{
    const qreal s = qMin(width() / qreal(LW), height() / qreal(LH));
    const qreal offX = (width()  - LW * s) / 2.0;
    const qreal offY = (height() - LH * s) / 2.0;
    return QPointF((p.x() - offX) / s, (p.y() - offY) / s);
}

void GameWidget::keyPressEvent(QKeyEvent *e)
{
    const int k = e->key();
    if (k == Qt::Key_M) { e->accept(); return; }  // (reserved / no-op: Qt build has no audio)

    if (k == Qt::Key_Escape) {
        if (m_state != Menu) { m_state = Menu; update(); }
        else close();
        e->accept();
        return;
    }
    if (k == Qt::Key_P) {
        if (m_state == Playing) m_state = Paused;
        else if (m_state == Paused) m_state = Playing;
        update();
        e->accept();
        return;
    }

    const bool up   = (k == Qt::Key_Up   || k == Qt::Key_W);
    const bool down = (k == Qt::Key_Down || k == Qt::Key_S);
    const bool confirm = (k == Qt::Key_Space || k == Qt::Key_Return || k == Qt::Key_Enter);

    if (m_state == Menu) {
        if (up)        { m_menuIndex = (m_menuIndex - 1 + modes().size()) % modes().size(); }
        else if (down) { m_menuIndex = (m_menuIndex + 1) % modes().size(); }
        else if (confirm) { startMode(m_menuIndex); }
        else if (k == Qt::Key_B) { m_state = Shop; }
        update();
        return;
    }
    if (m_state == Shop) {
        if (up)        { m_shopIndex = (m_shopIndex - 1 + skins().size()) % skins().size(); }
        else if (down) { m_shopIndex = (m_shopIndex + 1) % skins().size(); }
        else if (confirm) {
            const Skin &s = skins()[m_shopIndex];
            if (m_owned.contains(s.id)) { m_skinId = s.id; saveSkins(); }
            else if (m_coinsBalance >= s.cost) {
                m_coinsBalance -= s.cost; m_owned.insert(s.id); m_skinId = s.id;
                saveCoins(); saveSkins();
                m_shopMsg = QStringLiteral("已购买 %1！").arg(s.name); m_shopMsgLife = 1.6;
            } else { m_shopMsg = QStringLiteral("金币不足"); m_shopMsgLife = 1.6; }
        }
        update();
        return;
    }
    if (m_state == Paused) { if (confirm) m_state = Playing; update(); return; }
    if (m_state == GameOver) { if (confirm) startMode(m_modeIndex); update(); return; }

    // Ready / Playing -> flap
    if (confirm || up) {
        if (m_state == Ready) { m_state = Playing; flap(); }
        else if (m_state == Playing) flap();
        e->accept();
    }
}

void GameWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton) return;
    const QPointF p = toLogical(e->position());

    if (m_state == Menu) {
        for (int i = 0; i < modes().size(); ++i)
            if (menuCardRect(i).contains(p)) { m_menuIndex = i; startMode(i); update(); return; }
        if (menuShopBtnRect().contains(p)) { m_state = Shop; update(); return; }
        return;
    }
    if (m_state == Shop) {
        if (shopBackRect().contains(p)) { m_state = Menu; update(); return; }
        for (int i = 0; i < skins().size(); ++i) {
            if (shopItemRect(i).contains(p)) {
                m_shopIndex = i;
                const Skin &s = skins()[i];
                if (m_owned.contains(s.id)) { m_skinId = s.id; saveSkins(); }
                else if (m_coinsBalance >= s.cost) {
                    m_coinsBalance -= s.cost; m_owned.insert(s.id); m_skinId = s.id;
                    saveCoins(); saveSkins();
                    m_shopMsg = QStringLiteral("已购买 %1！").arg(s.name); m_shopMsgLife = 1.6;
                } else { m_shopMsg = QStringLiteral("金币不足"); m_shopMsgLife = 1.6; }
                update();
                return;
            }
        }
        return;
    }
    if (m_state == Paused) {
        if (pauseResumeRect().contains(p)) m_state = Playing;
        else if (pauseMenuRect().contains(p)) m_state = Menu;
        update();
        return;
    }
    if (m_state == GameOver) {
        if (overRetryRect().contains(p)) startMode(m_modeIndex);
        else if (overMenuRect().contains(p)) m_state = Menu;
        update();
        return;
    }
    if (m_state == Ready) { m_state = Playing; flap(); }
    else if (m_state == Playing) flap();
}

void GameWidget::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); }

// ==========================================================================
//  input regions
// ==========================================================================
QRectF GameWidget::menuCardRect(int i) const   { return QRectF(36, 124 + i * 70, LW - 72, 60); }
QRectF GameWidget::menuShopBtnRect() const     { return QRectF(36, 556, LW - 72, 46); }
QRectF GameWidget::shopItemRect(int i) const {
    const qreal cw = (LW - 3 * 24.0) / 2.0;
    const int col = i % 2, row = i / 2;
    return QRectF(24 + col * (cw + 24), 136 + row * 104, cw, 92);
}
QRectF GameWidget::shopBackRect() const        { return QRectF(LW / 2.0 - 80, 560, 160, 44); }
QRectF GameWidget::overRetryRect() const       { return QRectF(LW / 2.0 - 150, 510, 142, 48); }
QRectF GameWidget::overMenuRect() const        { return QRectF(LW / 2.0 + 8, 510, 142, 48); }
QRectF GameWidget::pauseResumeRect() const     { return QRectF(LW / 2.0 - 90, LH / 2.0 - 10, 180, 48); }
QRectF GameWidget::pauseMenuRect() const       { return QRectF(LW / 2.0 - 90, LH / 2.0 + 54, 180, 48); }

// ==========================================================================
//  painting
// ==========================================================================
void GameWidget::label(QPainter &p, const QRectF &r, const QString &t, int size,
                       QColor col, int flags, bool black, int shadow, qreal alpha)
{
    QFont f(QStringLiteral("Arial"), 10, black ? QFont::Black : QFont::DemiBold);
    f.setPixelSize(size);
    p.setFont(f);
    if (shadow > 0) {
        QColor sc(0, 0, 0, int(120 * alpha));
        p.setPen(sc);
        p.drawText(r.translated(1.5, 2.0), flags, t);
    }
    QColor c = col; c.setAlphaF(qBound(0.0, alpha, 1.0));
    p.setPen(c);
    p.drawText(r, flags, t);
}

void GameWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    const qreal s = qMin(width() / qreal(LW), height() / qreal(LH));
    const qreal offX = (width()  - LW * s) / 2.0;
    const qreal offY = (height() - LH * s) / 2.0;

    p.fillRect(rect(), QColor(18, 20, 40));
    p.translate(offX, offY);
    p.scale(s, s);
    p.setClipRect(0, 0, LW, LH);

    if (m_screenShake > 0)
        p.translate(frand(-1, 1) * m_screenShake, frand(-1, 1) * m_screenShake);

    drawSky(p);
    drawPipes(p);
    drawLasers(p);
    drawCoins(p);
    drawGround(p);
    drawParticles(p);
    if (m_state != Menu && m_state != Shop) drawBird(p);
    drawFloaters(p);

    if (mode().fog && (m_state == Playing || m_state == Paused ||
                       m_state == GameOver || m_state == Ready))
        drawFog(p);

    if (m_flash > 0)
        p.fillRect(QRectF(0, 0, LW, LH), QColor(230, 235, 255, int(m_flash * 150)));

    switch (m_state) {
    case Menu:     drawMenu(p);     break;
    case Shop:     drawShop(p);     break;
    case Ready:    drawReady(p);    break;
    case Playing:  drawHUD(p);      break;
    case Paused:   drawHUD(p); drawPaused(p);   break;
    case GameOver: drawHUD(p); drawGameOver(p); break;
    }
}

// ---- background -----------------------------------------------------------
static void skyStops(const QString &id, QColor &a, QColor &b, QColor &c)
{
    if (id == QLatin1String("zen"))       { a = QColor(255,215,168); b = QColor(255,184,200); c = QColor(201,179,255); }
    else if (id == QLatin1String("turbo")){ a = QColor( 58, 28, 82); b = QColor(161, 50, 90); c = QColor(255,138, 60); }
    else if (id == QLatin1String("night")){ a = QColor(  6, 11, 28); b = QColor( 15, 26, 56); c = QColor( 28, 43, 80); }
    else if (id == QLatin1String("storm")){ a = QColor( 20, 16, 38); b = QColor( 42, 30, 70); c = QColor( 58, 42, 99); }
    else if (id == QLatin1String("laser")){ a = QColor( 14, 12, 30); b = QColor( 40, 18, 46); c = QColor( 70, 26, 62); }
    else                                  { a = QColor( 74,166,232); b = QColor(143,208,240); c = QColor(216,240,248); }
}

void GameWidget::drawSky(QPainter &p)
{
    QColor a, b, c;
    skyStops(mode().id, a, b, c);
    QLinearGradient g(0, 0, 0, LH - GROUND_H);
    g.setColorAt(0.0, a); g.setColorAt(0.55, b); g.setColorAt(1.0, c);
    p.fillRect(QRectF(0, 0, LW, LH), g);

    const QString id = mode().id;
    const bool dark = (id == QLatin1String("night") || id == QLatin1String("storm") || id == QLatin1String("laser"));

    if (dark) {
        p.setPen(Qt::NoPen);
        for (const auto &st : m_stars) {
            const qreal al = (0.3 + 0.6 * qAbs(qSin(st.twinkle))) * (1.0 - st.y / (LH * 0.75));
            if (al <= 0.02) continue;
            p.setBrush(QColor(255, 255, 255, int(al * 255)));
            p.drawEllipse(QPointF(st.x, st.y), st.r, st.r);
        }
    }

    // sun / moon
    const qreal cx = LW - 86, cy = 104;
    QRadialGradient gr(cx, cy, 66);
    if (dark)                       { gr.setColorAt(0, QColor(235,240,255,240)); gr.setColorAt(0.5, QColor(190,205,240,90)); gr.setColorAt(1, QColor(190,205,240,0)); }
    else if (id == QLatin1String("turbo")) { gr.setColorAt(0, QColor(255,240,180,240)); gr.setColorAt(0.55, QColor(255,150,90,90)); gr.setColorAt(1, QColor(255,150,90,0)); }
    else                            { gr.setColorAt(0, QColor(255,248,210,240)); gr.setColorAt(0.55, QColor(255,225,150,90)); gr.setColorAt(1, QColor(255,225,150,0)); }
    p.setPen(Qt::NoPen); p.setBrush(gr);
    p.drawEllipse(QPointF(cx, cy), 66, 66);

    // parallax hills
    p.setBrush(dark ? QColor(255,255,255,13) : QColor(255,255,255,30));
    const qreal hy = LH - GROUND_H;
    for (const auto &h : m_hills) {
        qreal hx = std::fmod(h.x - m_bgOffset * 0.4, LW + 200.0);
        if (hx < -200) hx += LW + 200;
        QPainterPath path;
        path.moveTo(hx - h.w, hy);
        path.quadTo(hx, hy - h.h, hx + h.w, hy);
        path.closeSubpath();
        p.drawPath(path);
    }

    // clouds
    for (const auto &cl : m_clouds) {
        p.setBrush(dark ? QColor(90,106,144,40) : QColor(255,255,255,216));
        const qreal sc = cl.scale;
        p.drawEllipse(QPointF(cl.x,          cl.y),         26 * sc, 17 * sc);
        p.drawEllipse(QPointF(cl.x + 22 * sc, cl.y - 7 * sc), 22 * sc, 18 * sc);
        p.drawEllipse(QPointF(cl.x + 44 * sc, cl.y),         26 * sc, 16 * sc);
        p.drawEllipse(QPointF(cl.x + 20 * sc, cl.y + 6 * sc), 30 * sc, 15 * sc);
    }
}

void GameWidget::drawPipes(QPainter &p)
{
    const qreal bodyH = LH - GROUND_H;
    const qreal cap = 26;
    for (const auto &pipe : m_pipes) {
        const qreal top = pipeTopOf(pipe);
        const qreal by = top + pipe.gap;
        const int hue = pipe.hue;
        QLinearGradient g(pipe.x, 0, pipe.x + PIPE_W, 0);
        g.setColorAt(0.00, QColor::fromHsl(hue, 140, 82));
        g.setColorAt(0.28, QColor::fromHsl(hue, 170, 130));
        g.setColorAt(0.50, QColor::fromHsl(hue, 180, 148));
        g.setColorAt(0.72, QColor::fromHsl(hue, 150, 112));
        g.setColorAt(1.00, QColor::fromHsl(hue, 140, 70));
        QLinearGradient cg(pipe.x - 6, 0, pipe.x + PIPE_W + 6, 0);
        cg.setColorAt(0.0, QColor::fromHsl(hue, 150, 92));
        cg.setColorAt(0.5, QColor::fromHsl(hue, 180, 148));
        cg.setColorAt(1.0, QColor::fromHsl(hue, 150, 78));

        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRect(QRectF(pipe.x, 0, PIPE_W, top - cap));
        p.drawRect(QRectF(pipe.x, by + cap, PIPE_W, bodyH - by - cap));
        p.setBrush(cg);
        p.drawRoundedRect(QRectF(pipe.x - 6, top - cap, PIPE_W + 12, cap), 5, 5);
        p.drawRoundedRect(QRectF(pipe.x - 6, by, PIPE_W + 12, cap), 5, 5);

        p.setBrush(QColor(255, 255, 255, 70));
        p.drawRect(QRectF(pipe.x + 9, 0, 4, top - cap));
        p.drawRect(QRectF(pipe.x + 9, by + cap, 4, bodyH - by - cap));
    }
}

void GameWidget::drawLasers(QPainter &p)
{
    const qreal bodyH = LH - GROUND_H;
    for (const auto &l : m_lasers) {
        const qreal c = laserCenterOf(l);
        const qreal top = c - l.gap / 2, bot = c + l.gap / 2;
        const bool on = laserOn(l);
        const qreal ph = std::fmod(l.blink, 2.5);
        // flicker just before switching off
        qreal intensity = on ? (ph > 1.7 ? (0.4 + 0.6 * qAbs(qSin(l.blink * 40))) : 1.0) : 0.16;
        const qreal cxb = l.x + LASER_W / 2;

        // beam segments (top: 0..top, bottom: bot..bodyH)
        auto beam = [&](qreal y0, qreal y1) {
            QLinearGradient g(l.x, 0, l.x + LASER_W, 0);
            g.setColorAt(0.0, QColor(255, 60, 90, int(60 * intensity)));
            g.setColorAt(0.5, QColor(255, 210, 230, int(235 * intensity)));
            g.setColorAt(1.0, QColor(255, 60, 90, int(60 * intensity)));
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRect(QRectF(l.x, y0, LASER_W, y1 - y0));
            // hot core
            p.setBrush(QColor(255, 255, 255, int(220 * intensity)));
            p.drawRect(QRectF(cxb - 1.5, y0, 3, y1 - y0));
        };
        beam(0, top);
        beam(bot, bodyH);

        // emitter nodes framing the gap
        auto node = [&](qreal y) {
            QRadialGradient rg(cxb, y, 16);
            rg.setColorAt(0, QColor(255, 240, 245, int(255 * qMax(0.5, intensity))));
            rg.setColorAt(0.5, QColor(255, 80, 120, int(200 * qMax(0.4, intensity))));
            rg.setColorAt(1, QColor(255, 80, 120, 0));
            p.setBrush(rg);
            p.drawEllipse(QPointF(cxb, y), 16, 16);
            p.setBrush(QColor(60, 12, 22));
            p.drawRoundedRect(QRectF(l.x - 4, y - 7, LASER_W + 8, 14), 4, 4);
            p.setBrush(QColor(255, 90, 120, int(255 * qMax(0.4, intensity))));
            p.drawEllipse(QPointF(cxb, y), 4.5, 4.5);
        };
        node(top);
        node(bot);
    }
}

void GameWidget::drawCoins(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &c : m_coins) {
        if (c.collected) continue;
        const qreal wob = qAbs(qCos(c.spin));
        p.save();
        p.translate(c.x, c.y);
        p.setBrush(QColor(255, 220, 90, 64));
        p.drawEllipse(QPointF(0, 0), 15, 15);
        QLinearGradient g(-10, -10, 10, 10);
        g.setColorAt(0, QColor(255, 243, 176)); g.setColorAt(0.5, QColor(255, 207, 63)); g.setColorAt(1, QColor(229, 154, 16));
        p.setBrush(g);
        p.drawEllipse(QPointF(0, 0), 10 * (0.35 + 0.65 * wob), 10);
        p.setPen(QPen(QColor(168, 110, 8), 1.4));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(0, 0), 10 * (0.35 + 0.65 * wob), 10);
        p.setPen(Qt::NoPen);
        if (wob > 0.4) {
            p.setPen(QColor(168, 110, 8));
            QFont f(QStringLiteral("Arial")); f.setPixelSize(11); f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(-8, -8, 16, 16), Qt::AlignCenter, QStringLiteral("★"));
            p.setPen(Qt::NoPen);
        }
        p.restore();
    }
}

void GameWidget::drawGround(QPainter &p)
{
    const qreal y = LH - GROUND_H;
    const QString id = mode().id;
    const bool dark = (id == QLatin1String("night") || id == QLatin1String("storm") || id == QLatin1String("laser"));
    QLinearGradient g(0, y, 0, LH);
    if (dark) { g.setColorAt(0, QColor(58,90,58)); g.setColorAt(0.2, QColor(47,74,47)); g.setColorAt(1, QColor(36,58,36)); }
    else      { g.setColorAt(0, QColor(122,194,74)); g.setColorAt(0.18, QColor(154,216,94)); g.setColorAt(0.32, QColor(176,224,112)); g.setColorAt(1, QColor(91,138,56)); }
    p.fillRect(QRectF(0, y, LW, GROUND_H), g);

    p.setPen(QPen(QColor(255, 255, 255, 90), 2));
    p.drawLine(QPointF(0, y + 1), QPointF(LW, y + 1));

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 30));
    const qreal spacing = 46;
    const qreal off = std::fmod(m_groundOffset, spacing);
    for (qreal x = -off; x < LW; x += spacing) {
        p.drawRect(QRectF(x, y + 9, 22, 4));
        p.drawRect(QRectF(x + 12, y + 26, 18, 3));
        p.drawRect(QRectF(x + 5, y + 46, 20, 3));
    }
    QLinearGradient d(0, y + GROUND_H * 0.5, 0, LH);
    d.setColorAt(0, QColor(122, 86, 52, 230)); d.setColorAt(1, QColor(78, 52, 30, 242));
    p.setBrush(d);
    p.drawRect(QRectF(0, y + GROUND_H * 0.52, LW, GROUND_H * 0.48));
}

void GameWidget::drawBirdIcon(QPainter &p, qreal cx, qreal cy, qreal r, const Skin &s)
{
    QColor base = s.rainbow ? QColor::fromHsvF(std::fmod(m_tGlobal * 0.15, 1.0), 0.68, 1.0) : s.color;
    p.save();
    p.translate(cx, cy);
    if (s.glow) { p.setPen(Qt::NoPen); p.setBrush(QColor(base.red(), base.green(), base.blue(), 90)); p.drawEllipse(QPointF(0, 0), r + 6, r + 6); }
    QRadialGradient bg(-r * 0.3, -r * 0.3, r * 1.5);
    bg.setColorAt(0, shift(base, 40)); bg.setColorAt(0.6, base); bg.setColorAt(1, shift(base, -30));
    p.setPen(Qt::NoPen); p.setBrush(bg);
    p.drawEllipse(QPointF(0, 0), r, r);
    p.setBrush(QColor(255, 255, 255, 140));
    p.drawEllipse(QPointF(-r * 0.12, r * 0.28), r * 0.6, r * 0.4);
    // beak
    QLinearGradient bk(r, 0, r + r * 0.7, 0);
    bk.setColorAt(0, QColor(255, 154, 60)); bk.setColorAt(1, QColor(224, 100, 30));
    p.setBrush(bk);
    QPainterPath beak;
    beak.moveTo(r - r * 0.2, -r * 0.28); beak.lineTo(r + r * 0.7, r * 0.06); beak.lineTo(r - r * 0.2, r * 0.4);
    beak.closeSubpath(); p.drawPath(beak);
    // eye
    p.setBrush(Qt::white); p.drawEllipse(QPointF(r * 0.4, -r * 0.3), r * 0.4, r * 0.4);
    p.setBrush(QColor(26, 26, 36)); p.drawEllipse(QPointF(r * 0.52, -r * 0.3), r * 0.2, r * 0.2);
    p.restore();
}

void GameWidget::drawBird(QPainter &p)
{
    const QColor base = birdColor();
    p.save();
    p.translate(BIRD_X, m_birdY);
    p.rotate(qRadiansToDegrees(m_birdRot));

    if (mode().fog || skin().glow) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(base.red(), base.green(), base.blue(), 90));
        p.drawEllipse(QPointF(0, 0), BIRD_R + 8, BIRD_R + 8);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 56));
    p.drawEllipse(QPointF(3, 4), BIRD_R, BIRD_R * 0.9);

    QRadialGradient bg(-5, -5, BIRD_R * 1.5);
    bg.setColorAt(0, shift(base, 40)); bg.setColorAt(0.6, base); bg.setColorAt(1, shift(base, -30));
    p.setBrush(bg);
    p.drawEllipse(QPointF(0, 0), BIRD_R, BIRD_R);

    p.setBrush(QColor(255, 255, 255, 140));
    p.drawEllipse(QPointF(-2, 5), BIRD_R * 0.6, BIRD_R * 0.4);

    const qreal fl = (m_state == Playing) ? qSin(m_wingPhase) * 6.0 : qSin(m_tGlobal * 6) * 3.0;
    p.save();
    p.translate(-4, 1);
    p.rotate(-15 + fl);
    p.setBrush(shift(base, -18));
    p.drawEllipse(QPointF(0, 0), 11, 6.5);
    p.setBrush(shift(base, -34));
    p.drawEllipse(QPointF(0, 2), 8.5, 3.5);
    p.restore();

    p.setBrush(shift(base, -24));
    QPainterPath tail;
    tail.moveTo(-BIRD_R + 3, -2); tail.lineTo(-BIRD_R - 11, -9);
    tail.lineTo(-BIRD_R - 7, 0);  tail.lineTo(-BIRD_R - 11, 8);
    tail.closeSubpath();
    p.drawPath(tail);

    QLinearGradient beakG(BIRD_R, 0, BIRD_R + 12, 0);
    beakG.setColorAt(0, QColor(255, 154, 60)); beakG.setColorAt(1, QColor(224, 100, 30));
    p.setBrush(beakG);
    QPainterPath beak;
    beak.moveTo(BIRD_R - 3, -5); beak.lineTo(BIRD_R + 12, 1); beak.lineTo(BIRD_R - 3, 7);
    beak.closeSubpath();
    p.drawPath(beak);

    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(7, -5), 7, 7);
    p.setBrush(QColor(26, 26, 36));
    p.drawEllipse(QPointF(9, -5), 3.4, 3.4);
    p.setBrush(Qt::white);
    p.drawEllipse(QPointF(10.4, -6.4), 1.2, 1.2);

    p.restore();
}

void GameWidget::drawParticles(QPainter &p)
{
    p.setPen(Qt::NoPen);
    for (const auto &pt : m_particles) {
        QColor c = pt.color;
        c.setAlphaF(qBound(0.0, pt.life, 1.0));
        p.setBrush(c);
        p.drawEllipse(QPointF(pt.x, pt.y), pt.size, pt.size);
    }
}
void GameWidget::drawFloaters(QPainter &p)
{
    for (const auto &f : m_floaters)
        label(p, QRectF(f.x - 60, f.y - 12, 120, 24), f.text, 18, f.color,
              Qt::AlignCenter, true, 6, qMax(0.0, f.life));
}

void GameWidget::drawFog(QPainter &p)
{
    const qreal r = 150;
    QRadialGradient g(BIRD_X, m_birdY, r);
    g.setColorAt(0.0,  QColor(4, 8, 20, 0));
    g.setColorAt(0.7,  QColor(4, 8, 20, 90));
    g.setColorAt(1.0,  QColor(3, 6, 16, 235));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRect(QRectF(0, 0, LW, LH - GROUND_H));
}

// ---- HUD & screens --------------------------------------------------------
void GameWidget::drawHUD(QPainter &p)
{
    p.save();
    p.translate(LW / 2.0, 78);
    const qreal sc = 1 + m_scorePop * 0.35;
    p.scale(sc, sc);
    label(p, QRectF(-120, -34, 240, 68), QString::number(m_score), 52,
          QColor(255, 255, 255), Qt::AlignCenter, true, 10);
    p.restore();

    // mode badge
    p.setPen(Qt::NoPen); p.setBrush(QColor(0, 0, 0, 72));
    p.drawRoundedRect(QRectF(12, 14, 118, 30), 15, 15);
    label(p, QRectF(24, 14, 110, 30), mode().icon + QStringLiteral(" ") + mode().name, 15,
          mode().accent, Qt::AlignVCenter | Qt::AlignLeft);

    // coins
    p.setBrush(QColor(0, 0, 0, 72));
    p.drawRoundedRect(QRectF(LW - 118, 14, 106, 30), 15, 15);
    label(p, QRectF(LW - 118, 14, 90, 30), QStringLiteral("★ %1").arg(m_runCoins), 15,
          QColor(255, 226, 122), Qt::AlignVCenter | Qt::AlignHCenter);

    if (m_combo >= 3)
        label(p, QRectF(LW - 150, 48, 138, 20), QStringLiteral("连击 ×%1").arg(m_combo), 13,
              QColor(255, 217, 61), Qt::AlignVCenter | Qt::AlignRight, true, 6);

    if (mode().storm && (m_gustWarn > 0 || m_gustActive)) {
        QString t; QColor col;
        if (m_gustActive) { t = m_gustDir < 0 ? QStringLiteral("↑ 上升气流") : QStringLiteral("↓ 下沉气流"); col = QColor(197,139,255); }
        else { t = QStringLiteral("⚠ 阵风来袭"); col = QColor(255,217,61); }
        const qreal a = m_gustActive ? 1.0 : (0.4 + 0.6 * qAbs(qSin(m_tGlobal * 12)));
        label(p, QRectF(0, 104, LW, 28), t, 15, col, Qt::AlignHCenter, true, 8, a);
    }
}

void GameWidget::drawReady(QPainter &p)
{
    p.fillRect(QRectF(0, 0, LW, LH), QColor(0, 0, 0, 46));
    label(p, QRectF(0, LH / 2.0 - 118, LW, 40), mode().icon + QStringLiteral(" ") + mode().name,
          30, mode().accent, Qt::AlignHCenter, true, 10);
    label(p, QRectF(0, LH / 2.0 - 78, LW, 24), mode().desc, 15,
          QColor(255, 255, 255, 220), Qt::AlignHCenter, false);
    const qreal a = 0.5 + 0.5 * qSin(m_tGlobal * 5);
    label(p, QRectF(0, LH / 2.0 + 54, LW, 28), QStringLiteral("点击 / 空格 起飞"), 18,
          QColor(255, 255, 255), Qt::AlignHCenter, true, 8, a);
    label(p, QRectF(0, LH / 2.0 + 92, LW, 22), QStringLiteral("最佳 %1").arg(getBest(mode().id)), 14,
          QColor(255, 255, 255, 180), Qt::AlignHCenter, false);
}

void GameWidget::drawMenu(QPainter &p)
{
    label(p, QRectF(0, 44, LW, 52), QStringLiteral("FLAPPY BIRD"), 42,
          QColor(255, 255, 255), Qt::AlignHCenter, true, 14);
    label(p, QRectF(0, 96, LW, 22), QStringLiteral("多模式 · 皮肤商店 · 激光门"), 14,
          QColor(255, 255, 255, 200), Qt::AlignHCenter, false);

    // coins top-right
    label(p, QRectF(LW - 150, 20, 138, 24), QStringLiteral("★ %1").arg(m_coinsBalance), 15,
          QColor(255, 226, 122), Qt::AlignRight | Qt::AlignVCenter, true, 6);

    for (int i = 0; i < modes().size(); ++i) {
        const GameMode &m = modes()[i];
        const QRectF r = menuCardRect(i);
        const bool sel = (i == m_menuIndex);
        const qreal pulse = sel ? (0.5 + 0.5 * qSin(m_tGlobal * 5)) : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(sel ? QColor(22, 28, 54, 210) : QColor(22, 28, 54, 145));
        p.drawRoundedRect(r, 15, 15);
        QColor bc = sel ? m.accent : QColor(255, 255, 255, 46);
        bc.setAlphaF(sel ? (0.7 + pulse * 0.3) : 0.4);
        p.setPen(QPen(bc, sel ? 2.4 : 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 15, 15);

        // icon chip
        QColor chip = m.accent; chip.setAlphaF(0.22);
        p.setPen(Qt::NoPen); p.setBrush(chip);
        p.drawRoundedRect(QRectF(r.x() + 10, r.y() + 9, 42, 42), 12, 12);
        label(p, QRectF(r.x() + 10, r.y() + 9, 42, 42), m.icon, 24, QColor(255, 255, 255), Qt::AlignCenter);

        label(p, QRectF(r.x() + 62, r.y() + 8, 220, 26), m.name, 18,
              sel ? m.accent : QColor(255, 255, 255), Qt::AlignVCenter | Qt::AlignLeft);
        label(p, QRectF(r.x() + 62, r.y() + 32, 260, 22), m.desc, 12,
              QColor(255, 255, 255, 190), Qt::AlignVCenter | Qt::AlignLeft, false);

        label(p, QRectF(r.right() - 76, r.y() + 6, 64, 18), QStringLiteral("最佳"), 11,
              QColor(255, 255, 255, 140), Qt::AlignVCenter | Qt::AlignLeft, false);
        label(p, QRectF(r.right() - 76, r.y() + 24, 64, 26), QString::number(getBest(m.id)), 20,
              QColor(255, 255, 255), Qt::AlignVCenter | Qt::AlignLeft);
    }

    // shop button
    const QRectF sb = menuShopBtnRect();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 226, 122, 40));
    p.drawRoundedRect(sb, 14, 14);
    p.setPen(QPen(QColor(255, 226, 122, 200), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(sb, 14, 14);
    label(p, sb, QStringLiteral("🛒  皮肤商店"), 18, QColor(255, 226, 122), Qt::AlignCenter, true, 4);

    const qreal a = 0.5 + 0.5 * qSin(m_tGlobal * 4);
    label(p, QRectF(0, 612, LW, 20), QStringLiteral("↑↓ 选择 · 回车开始 · B 进商店 · 点击卡片直接玩"),
          12, QColor(255, 255, 255, 210), Qt::AlignHCenter, false, 0, a);
}

void GameWidget::drawShop(QPainter &p)
{
    p.fillRect(QRectF(0, 0, LW, LH), QColor(14, 16, 34, 205));
    label(p, QRectF(0, 40, LW, 34), QStringLiteral("皮肤商店"), 30,
          QColor(255, 255, 255), Qt::AlignHCenter, true, 10);
    label(p, QRectF(LW - 160, 96, 148, 24), QStringLiteral("★ %1").arg(m_coinsBalance), 16,
          QColor(255, 226, 122), Qt::AlignRight | Qt::AlignVCenter, true, 6);
    label(p, QRectF(12, 96, 200, 24), QStringLiteral("金币余额"), 13,
          QColor(255, 255, 255, 160), Qt::AlignLeft | Qt::AlignVCenter, false);

    for (int i = 0; i < skins().size(); ++i) {
        const Skin &s = skins()[i];
        const QRectF r = shopItemRect(i);
        const bool owned = m_owned.contains(s.id);
        const bool equipped = (s.id == m_skinId);
        const bool sel = (i == m_shopIndex);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, equipped ? 34 : 20));
        p.drawRoundedRect(r, 14, 14);
        QColor border = equipped ? QColor(126, 232, 176)
                       : sel     ? QColor(255, 255, 255, 200)
                                 : QColor(255, 255, 255, 46);
        p.setPen(QPen(border, equipped || sel ? 2.4 : 1.2));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r, 14, 14);

        drawBirdIcon(p, r.center().x(), r.y() + 34, 18, s);
        label(p, QRectF(r.x(), r.y() + 54, r.width(), 20), s.name, 15,
              QColor(255, 255, 255), Qt::AlignHCenter);

        QString status; QColor col;
        if (equipped)     { status = QStringLiteral("已装备");  col = QColor(126, 232, 176); }
        else if (owned)   { status = QStringLiteral("点击装备"); col = QColor(255, 255, 255, 200); }
        else if (m_coinsBalance >= s.cost) { status = QStringLiteral("★ %1").arg(s.cost); col = QColor(255, 226, 122); }
        else              { status = QStringLiteral("★ %1").arg(s.cost); col = QColor(255, 255, 255, 110); }
        label(p, QRectF(r.x(), r.y() + 72, r.width(), 18), status, 13, col, Qt::AlignHCenter, true);
    }

    // toast message
    if (m_shopMsgLife > 0)
        label(p, QRectF(0, 526, LW, 24), m_shopMsg, 15, QColor(255, 217, 61),
              Qt::AlignHCenter, true, 6, qMin(1.0, m_shopMsgLife));

    // back button
    const QRectF bk = shopBackRect();
    p.setPen(QPen(QColor(255, 255, 255, 200), 2));
    p.setBrush(QColor(255, 255, 255, 22));
    p.drawRoundedRect(bk, 14, 14);
    label(p, bk, QStringLiteral("← 返回菜单"), 17, QColor(255, 255, 255), Qt::AlignCenter, true);
}

void GameWidget::drawPaused(QPainter &p)
{
    p.fillRect(QRectF(0, 0, LW, LH), QColor(0, 0, 0, 140));
    label(p, QRectF(0, LH / 2.0 - 84, LW, 44), QStringLiteral("⏸ 暂停"), 34,
          QColor(255, 255, 255), Qt::AlignHCenter, true, 10);

    const QRectF rr = pauseResumeRect(), rm = pauseMenuRect();
    p.setPen(QPen(mode().accent, 2)); p.setBrush(QColor(mode().accent.red(), mode().accent.green(), mode().accent.blue(), 40));
    p.drawRoundedRect(rr, 14, 14);
    label(p, rr, QStringLiteral("继续"), 17, mode().accent, Qt::AlignCenter, true);
    p.setPen(QPen(QColor(255, 255, 255, 200), 2)); p.setBrush(QColor(255, 255, 255, 22));
    p.drawRoundedRect(rm, 14, 14);
    label(p, rm, QStringLiteral("返回菜单"), 17, QColor(255, 255, 255), Qt::AlignCenter, true);
}

void GameWidget::drawGameOver(QPainter &p)
{
    p.fillRect(QRectF(0, 0, LW, LH), QColor(0, 0, 0, 140));
    const qreal bw = 320, bh = 300;
    const qreal bx = (LW - bw) / 2.0, by = (LH - bh) / 2.0 - 16;

    QLinearGradient panel(bx, by, bx, by + bh);
    panel.setColorAt(0, QColor(48, 52, 96, 245)); panel.setColorAt(1, QColor(26, 30, 58, 245));
    p.setPen(QPen(QColor(mode().accent.red(), mode().accent.green(), mode().accent.blue(), 180), 2));
    p.setBrush(panel);
    p.drawRoundedRect(QRectF(bx, by, bw, bh), 22, 22);

    label(p, QRectF(bx, by + 22, bw, 40), QStringLiteral("游戏结束"), 28,
          QColor(255, 122, 122), Qt::AlignHCenter, true, 8);

    QString medal; QColor mc;
    if      (m_score >= 40) { medal = QStringLiteral("🏅 大师"); mc = QColor(127, 211, 255); }
    else if (m_score >= 25) { medal = QStringLiteral("🥇 金牌"); mc = QColor(255, 217, 61); }
    else if (m_score >= 12) { medal = QStringLiteral("🥈 银牌"); mc = QColor(223, 230, 240); }
    else if (m_score >= 5)  { medal = QStringLiteral("🥉 铜牌"); mc = QColor(217, 160, 102); }
    else                    { medal = QStringLiteral("🐣 新手"); mc = QColor(190, 205, 220); }
    label(p, QRectF(bx, by + 62, bw, 34), medal, 22, mc, Qt::AlignHCenter, true, 6);

    auto stat = [&](qreal cxp, qreal yy, const QString &lab, const QString &val, QColor col) {
        label(p, QRectF(cxp - 60, yy, 120, 18), lab, 12, QColor(255, 255, 255, 160), Qt::AlignHCenter, false);
        label(p, QRectF(cxp - 60, yy + 18, 120, 28), val, 22, col, Qt::AlignHCenter, true, 4);
    };
    const qreal sy = by + 116;
    stat(bx + bw * 0.27, sy, QStringLiteral("得分"), QString::number(m_score), QColor(255, 255, 255));
    stat(bx + bw * 0.50, sy, QStringLiteral("金币"), QStringLiteral("★%1").arg(m_runCoins), QColor(255, 226, 122));
    stat(bx + bw * 0.73, sy, QStringLiteral("最佳"), QString::number(getBest(mode().id)), mode().accent);
    const qreal sy2 = by + 174;
    stat(bx + bw * 0.32, sy2, QStringLiteral("连击"), QStringLiteral("×%1").arg(m_bestCombo), QColor(255, 217, 61));
    stat(bx + bw * 0.68, sy2, QStringLiteral("存活"), QStringLiteral("%1s").arg(m_elapsedSec, 0, 'f', 1), QColor(159, 230, 192));

    if (m_newRecord) {
        const qreal a = 0.6 + 0.4 * qSin(m_tGlobal * 8);
        label(p, QRectF(bx, by + 232, bw, 24), QStringLiteral("🎉 新纪录！"), 16,
              QColor(255, 217, 61), Qt::AlignHCenter, true, 6, a);
    }

    const QRectF rr = overRetryRect(), rm = overMenuRect();
    p.setPen(QPen(mode().accent, 2)); p.setBrush(QColor(mode().accent.red(), mode().accent.green(), mode().accent.blue(), 40));
    p.drawRoundedRect(rr, 14, 14);
    label(p, rr, QStringLiteral("再来一局"), 17, mode().accent, Qt::AlignCenter, true);
    p.setPen(QPen(QColor(255, 255, 255, 200), 2)); p.setBrush(QColor(255, 255, 255, 22));
    p.drawRoundedRect(rm, 14, 14);
    label(p, rm, QStringLiteral("返回菜单"), 17, QColor(255, 255, 255), Qt::AlignCenter, true);
}
