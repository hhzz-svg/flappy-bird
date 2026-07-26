#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QVector>
#include <QString>
#include <QColor>
#include <QRectF>
#include <QSet>
#include <QElapsedTimer>

class QTimer;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;

// --- Static definitions --------------------------------------------------
struct GameMode {
    QString id, name, icon, desc;
    QColor  accent;
    qreal   gravity, jump, gap, speed;
    int     spawnMs;
    qreal   scoreMul, ramp;
    bool    movingPipes, fog, storm, lasers;
    qreal   coinRate;
};

struct Skin {
    QString id, name;
    int     cost;
    QColor  color;
    bool    rainbow;
    bool    glow;
};

// --- Runtime objects -----------------------------------------------------
struct Pipe {
    qreal x, baseTop, gap;
    bool  scored;
    qreal amp, phase, freq;   // moving-pipe oscillation (storm)
    int   hue;
};

struct Laser {
    qreal x, baseCenter, gap;
    bool  scored;
    qreal amp, phase, freq;   // vertical oscillation (moving gate)
    qreal blink;              // blink phase; laser turns off briefly
};

struct Coin {
    qreal x, y;
    bool  collected;
    qreal spin;
};

struct Particle {
    qreal x, y, vx, vy, life, decay, size;
    QColor color;
};

struct Floater {
    qreal x, y, life;
    QString text;
    QColor color;
};

struct Star  { qreal x, y, r, twinkle, speed; };
struct Cloud { qreal x, y, scale, speed; };
struct Hill  { qreal x, w, h; };

class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void tick();

private:
    enum State { Menu, Shop, Ready, Playing, Paused, GameOver };

    // --- data tables ---
    static const QVector<GameMode> &modes();
    static const QVector<Skin>     &skins();
    const GameMode &mode() const { return modes()[m_modeIndex]; }
    const Skin     &skin() const;
    QColor birdColor() const;

    // --- lifecycle ---
    void resetRun();
    void startMode(int index);
    void flap();
    void die();
    void spawnPipe();
    void spawnLaser();
    void spawnParticles(qreal x, qreal y, int count, const QVector<QColor> &cols, qreal spread = 5.0);
    void addFloater(qreal x, qreal y, const QString &t, QColor c);

    // --- helpers ---
    qreal curSpeed() const;
    qreal curGap() const;
    int   curSpawnMs() const;
    qreal pipeTopOf(const Pipe &p) const;
    qreal laserCenterOf(const Laser &l) const;
    bool  laserOn(const Laser &l) const;
    bool  checkCollision();
    QPointF toLogical(const QPointF &p) const;

    // --- persistence ---
    void loadProfile();
    void saveBest();
    void saveCoins();
    void saveSkins();

    // --- input regions (logical coords, single source of truth) ---
    QRectF menuCardRect(int i) const;
    QRectF menuShopBtnRect() const;
    QRectF shopItemRect(int i) const;
    QRectF shopBackRect() const;
    QRectF overRetryRect() const;
    QRectF overMenuRect() const;
    QRectF pauseResumeRect() const;
    QRectF pauseMenuRect() const;

    // --- drawing ---
    void drawSky(QPainter &p);
    void drawPipes(QPainter &p);
    void drawLasers(QPainter &p);
    void drawCoins(QPainter &p);
    void drawGround(QPainter &p);
    void drawBird(QPainter &p);
    void drawParticles(QPainter &p);
    void drawFloaters(QPainter &p);
    void drawFog(QPainter &p);
    void drawHUD(QPainter &p);
    void drawReady(QPainter &p);
    void drawMenu(QPainter &p);
    void drawShop(QPainter &p);
    void drawPaused(QPainter &p);
    void drawGameOver(QPainter &p);
    void drawBirdIcon(QPainter &p, qreal cx, qreal cy, qreal r, const Skin &s);
    void label(QPainter &p, const QRectF &r, const QString &t, int size,
               QColor col, int flags, bool black = true, int shadow = 0, qreal alpha = 1.0);

    // Logical canvas
    static constexpr int   LW = 480;
    static constexpr int   LH = 720;
    static constexpr qreal GROUND_H = 84.0;
    static constexpr qreal BIRD_X   = 118.0;
    static constexpr qreal BIRD_R   = 17.0;
    static constexpr qreal PIPE_W   = 66.0;
    static constexpr qreal LASER_W  = 16.0;
    static constexpr qreal MAX_V    = 11.0;

    // State
    State m_state;
    int   m_modeIndex;
    int   m_menuIndex;
    int   m_shopIndex;

    // Bird
    qreal m_birdY, m_birdV, m_birdRot, m_wingPhase;

    // World
    QVector<Pipe>     m_pipes;
    QVector<Laser>    m_lasers;
    QVector<Coin>     m_coins;
    QVector<Particle> m_particles;
    QVector<Floater>  m_floaters;
    QVector<Star>     m_stars;
    QVector<Cloud>    m_clouds;
    QVector<Hill>     m_hills;

    qreal m_groundOffset, m_bgOffset, m_tGlobal;
    int   m_score, m_runCoins, m_combo, m_bestCombo;
    qreal m_elapsedSec;
    qreal m_screenShake, m_scorePop, m_flash;
    int   m_msSincePipe;
    bool  m_newRecord;

    // storm wind
    bool  m_gustActive;
    qreal m_gustTimer, m_gustAccel, m_gustWarn;
    int   m_gustDir;

    // profile
    int         m_coinsBalance;   // lifetime coins (shop currency)
    QSet<QString> m_owned;
    QString     m_skinId;
    QString     m_shopMsg;
    qreal       m_shopMsgLife;

    QTimer        *m_timer;
    QElapsedTimer  m_elapsed;
};

#endif
