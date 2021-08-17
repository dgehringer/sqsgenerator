

class BadSettings(Exception):

    def __init__(self, message=None, parameter=None):
        super(BadSettings, self).__init__(message)
        self.message = message
        self.parameter = parameter