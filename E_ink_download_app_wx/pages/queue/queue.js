const app = getApp()

const STATUS_MAP = {
  pending:     { text: '等待中',  cls: 'pending' },
  downloading: { text: '下载中',  cls: 'downloading' },
  done:        { text: '已完成',  cls: 'done' },
  cancelled:   { text: '已取消',  cls: 'cancelled' }
}

Page({
  data: {
    queue: [],
    loading: false
  },

  _pollTimer: null,
  _etaTimer:  null,
  _startMs:   {},   /* taskId → 开始下载时的本地毫秒时间戳，用于 ETA 计算 */

  onShow() {
    this.loadQueue()
    this._startPollTimer()
    this._startEtaTimer()
  },

  onHide() {
    this._stopPollTimer()
    this._stopEtaTimer()
  },

  onUnload() {
    this._stopPollTimer()
    this._stopEtaTimer()
  },

  /* ── 3 秒服务端轮询（只在有活跃任务时才请求）── */
  _startPollTimer() {
    this._stopPollTimer()
    this._pollTimer = setInterval(() => {
      const hasActive = this.data.queue.some(i =>
        i.statusCls === 'downloading' || i.statusCls === 'pending')
      if (hasActive) this.loadQueue()
    }, 3000)
  },
  _stopPollTimer() {
    if (this._pollTimer) { clearInterval(this._pollTimer); this._pollTimer = null }
  },

  /* ── 1 秒本地 ETA 定时器（纯计算，不请求网络，避免刷新闪烁）── */
  _startEtaTimer() {
    this._stopEtaTimer()
    this._etaTimer = setInterval(() => this._tickEta(), 1000)
  },
  _stopEtaTimer() {
    if (this._etaTimer) { clearInterval(this._etaTimer); this._etaTimer = null }
  },

  _tickEta() {
    const queue = this.data.queue
    const updates = {}
    queue.forEach((item, i) => {
      if (item.statusCls !== 'downloading') return
      const pct = item.progress || 0
      if (pct <= 0) { updates[`queue[${i}].eta`] = '计算中…'; return }
      if (pct >= 100) return

      /* 首次见到该任务：优先用 started_at_unix，其次 +08:00 字符串，最后本地时间 */
      if (!this._startMs[item.id]) {
        if (item.started_at_unix) {
          this._startMs[item.id] = item.started_at_unix * 1000
        } else if (item.started_at) {
          const t = new Date(item.started_at.replace(' ', 'T') + '+08:00').getTime()
          this._startMs[item.id] = isNaN(t) ? Date.now() : t
        } else {
          this._startMs[item.id] = Date.now()
        }
      }

      const elapsedSec = (Date.now() - this._startMs[item.id]) / 1000
      if (elapsedSec <= 0) return
      const remainSec = elapsedSec * (100 - pct) / pct
      const eta = this._fmtTime(remainSec)
      if (item.eta !== eta) updates[`queue[${i}].eta`] = eta
    })
    if (Object.keys(updates).length) this.setData(updates)
  },

  _fmtTime(sec) {
    if (!isFinite(sec) || sec < 0) return '计算中…'
    sec = Math.round(sec)
    if (sec < 60) return `约 ${sec} 秒`
    const m = Math.floor(sec / 60), s = sec % 60
    return s > 0 ? `约 ${m} 分 ${s} 秒` : `约 ${m} 分钟`
  },

  onPullDownRefresh() {
    this.loadQueue(() => wx.stopPullDownRefresh())
  },

  cancelTask(e) {
    const id = e.currentTarget.dataset.id
    wx.showModal({
      title: '取消下载',
      content: '确认取消该任务？正在传输中的任务本次仍会完成。',
      confirmColor: '#e53935',
      success: (res) => {
        if (!res.confirm) return
        wx.request({
          url: `${app.globalData.apiBase}/queue.php?action=cancel&id=${id}`,
          success: (r) => {
            if (r.data && r.data.status === 'ok') {
              wx.showToast({ title: '已取消', icon: 'success' })
              this.loadQueue()
            } else {
              wx.showToast({ title: '取消失败', icon: 'error' })
            }
          },
          fail: () => wx.showToast({ title: '网络错误', icon: 'error' })
        })
      }
    })
  },

  loadQueue(callback) {
    this.setData({ loading: true })
    wx.request({
      url: `${app.globalData.apiBase}/queue.php?action=list`,
      success: (res) => {
        if (res.data && res.data.status === 'ok') {
          const queue = (res.data.queue || []).map(item => {
            const s = STATUS_MAP[item.status] || { text: item.status, cls: 'pending' }

            /* 预填充起始时间：优先用 started_at_unix（Unix 时间戳，无时区问题）*/
            if (item.status === 'downloading' && !this._startMs[item.id]) {
              if (item.started_at_unix) {
                this._startMs[item.id] = item.started_at_unix * 1000
              } else if (item.started_at) {
                const t = new Date(item.started_at.replace(' ', 'T') + '+08:00').getTime()
                if (!isNaN(t)) this._startMs[item.id] = t
              }
            }
            /* 任务结束时释放内存 */
            if (item.status === 'done' || item.status === 'cancelled') {
              delete this._startMs[item.id]
            }

            /* title 用书名；fallback 去掉 .txt 后缀 */
            const title = item.title ||
              (item.filename ? item.filename.replace(/\.txt$/i, '') : `任务 #${item.id}`)

            return Object.assign({}, item, {
              title,
              statusText: s.text,
              statusCls:  s.cls,
              progress:   item.progress || 0,
              eta:        '',
              queuedAt:   (item.queued_at || '').substring(0, 16),
              doneAt:     (item.done_at   || '').substring(0, 16)
            })
          })

          /* 整体替换队列后立即计算一次 ETA，减少显示延迟 */
          this.setData({ queue }, () => this._tickEta())
        } else {
          wx.showToast({ title: '加载失败', icon: 'error' })
        }
      },
      fail: () => wx.showToast({ title: '网络错误', icon: 'error' }),
      complete: () => {
        this.setData({ loading: false })
        callback && callback()
      }
    })
  }
})
