module.exports = [
  {
    type: 'heading',
    defaultValue: 'Wrist Cast'
  },
  {
    type: 'text',
    defaultValue: 'Tune casts, bites, and haptics for idle wrist fishing.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Haptics'
      },
      {
        type: 'toggle',
        messageKey: 'VibeEnabled',
        label: 'Vibrate',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'QuietHours',
        label: 'Quiet hours (10pm–7am)',
        defaultValue: false
      }
    ]
  },
  {
    type: 'section',
    items: [
      {
        type: 'heading',
        defaultValue: 'Play'
      },
      {
        type: 'select',
        messageKey: 'WaitLength',
        label: 'Bite wait',
        defaultValue: '1',
        options: [
          { label: 'Short', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'Long', value: '2' }
        ]
      },
      {
        type: 'select',
        messageKey: 'CastSensitivity',
        label: 'Cast sensitivity',
        defaultValue: '1',
        options: [
          { label: 'Low', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'High', value: '2' }
        ]
      },
      {
        type: 'select',
        messageKey: 'FightDifficulty',
        label: 'Fight difficulty',
        defaultValue: '1',
        options: [
          { label: 'Easy', value: '0' },
          { label: 'Normal', value: '1' },
          { label: 'Hard', value: '2' }
        ]
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
