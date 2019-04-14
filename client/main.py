#!/usr/bin/env python3

from kivy.app import App
from kivy.uix.widget import Widget
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.screenmanager import ScreenManager, Screen
import time

class CalculusLoadScreen(Screen):
    pass

class CalculusGameScreen(Screen):
    def takeFrom(self, stack):
        self.ids.feedback.text = "You've taken\nfrom stack" + str(stack)

class CalculusWinScreen(Screen):
    pass

class CalculusLostScreen(Screen):
    pass

################################################################


sm = ScreenManager()
sm.add_widget(CalculusLoadScreen(name='load'))
sm.add_widget(CalculusGameScreen(name='game'))
sm.add_widget(CalculusWinScreen(name='win'))
sm.add_widget(CalculusLostScreen(name='lost'))

class CalculusApp(App):
    def build(self):
        return sm

    sm.current='game'

################################################################

if __name__ == '__main__':
    CalculusApp().run()

